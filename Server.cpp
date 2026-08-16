#include "Server.hpp"
#include "Command.hpp"

#include <cctype>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

/* ========================================================================== */
/*                              construction                                  */
/* ========================================================================== */

Server::Server(int port, const std::string &pass) : _lfd(-1), _port(port), _password(pass)
{
	_handlers["PASS"] = &Server::checkPass;
	_handlers["NICK"] = &Server::setNickname;
	_handlers["USER"] = &Server::setUsername;
	_handlers["QUIT"] = &Server::disconnect;
	_handlers["PING"] = &Server::ping;
    _handlers["JOIN"] = &Server::joinChannel;
}

Server::~Server()
{
	if (_lfd >= 0)
		close(_lfd);
}

/* ========================================================================== */
/*                                 setup                                      */
/* ========================================================================== */

void	Server::setup()
{
	sockaddr_in s;

	memset(&s, 0, sizeof(s));
	s.sin_family = AF_INET;
	s.sin_addr.s_addr = INADDR_ANY;
	s.sin_port = htons(_port);

	_lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (_lfd < 0)
		throw std::runtime_error(std::string("Socket creation failed: ") + std::strerror(errno));

	int opt = 1;
	if (setsockopt(_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw std::runtime_error(std::string("setsockopt failed: ") + std::strerror(errno));
	if (bind(_lfd, (struct sockaddr *)&s, sizeof(s)) < 0)
		throw std::runtime_error(std::string("Bind failed: ") + std::strerror(errno));
	if (listen(_lfd, 10) < 0)
		throw std::runtime_error(std::string("Listen failed: ") + std::strerror(errno));
	if (fcntl(_lfd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error(std::string("fcntl failed: ") + std::strerror(errno));
}

/* ========================================================================== */
/*                           write path (POLLOUT)                             */
/* ========================================================================== */

const std::string Server::getMembersList(std::string channelName)
{
    std::string membersList;
    std::map<std::string, Channel>::iterator channelIt = _channels.find(toUppercase(channelName));
    if (channelIt == _channels.end())
        return membersList;
    for (std::set<int>::iterator it = channelIt->second.getOperators().begin(); it != channelIt->second.getOperators().end(); ++it)
    {
        membersList += "@" + _clients[*it].getNick();
        if (*it != *channelIt->second.getOperators().rbegin() || !channelIt->second.getMembers().empty())
            membersList += " ";
    }
    for (std::set<int>::iterator it = channelIt->second.getMembers().begin(); it != channelIt->second.getMembers().end(); ++it)
    {
        membersList += _clients[*it].getNick();
        if (*it != *channelIt->second.getMembers().rbegin())
            membersList += " ";
    }
    return membersList;
}

void    Server::broadcastMessage(const std::string& channelName, const std::string &message)
{
    std::map<std::string, Channel>::const_iterator it = _channels.find(channelName);
    if (it == _channels.end())
        return;
    for (std::set<int>::iterator opIt = it->second.getOperators().begin(); opIt != it->second.getOperators().end(); ++opIt)
        sendToClient(message, *opIt);
    for (std::set<int>::iterator memberIt = it->second.getMembers().begin(); memberIt != it->second.getMembers().end(); ++memberIt)
        sendToClient(message, *memberIt);
}

void	Server::sendToClient(const std::string &message, int fd)
{
	std::map<int, Client>::iterator it = _clients.find(fd);

	if (it == _clients.end())
		return ;
	it->second.appendWriteBuffer(message);
}

void	Server::flushClient(int fd)
{
	std::map<int, Client>::iterator it = _clients.find(fd);

	if (it == _clients.end() || !it->second.hasPendingWrite())
		return ;

	std::string	out = it->second.getWriteBuffer();
	ssize_t		n = send(fd, out.c_str(), out.size(), 0);

	if (n > 0)
		it->second.consumeWriteBuffer(static_cast<size_t>(n));
	else if (n < 0)
		it->second.setWriteBuffer("");
}

/* ========================================================================== */
/*                            parsing + dispatch                              */
/* ========================================================================== */

std::string	toUppercase(std::string str)
{
	for (size_t i = 0; i < str.size(); i++)
		str[i] = static_cast<unsigned char>(std::toupper(str[i]));
    return str;
}

std::vector<std::string>	Server::split_cmd(const std::string &cmd)
{
	std::vector<std::string>	tokens;
	std::istringstream			stream(cmd);
	std::string					token;

	while (stream >> token)
	{
		if (token[0] == ':')
			break;
		tokens.push_back(token);
		token.clear();
	}
	return tokens;
}

Command	Server::buildCommand(const std::string &line)
{
	Command	cmds;

	cmds.args = split_cmd(line);
	if (cmds.args.empty())
		return cmds;

	cmds.command = cmds.args[0];
	cmds.command = toUppercase(cmds.command);
	cmds.args.erase(cmds.args.begin());

	if (line.find(" :") != std::string::npos)
		cmds.trailing = line.substr(line.find(" :") + 2);
	if (!cmds.trailing.empty())
		cmds.args.push_back(cmds.trailing);
	return cmds;
}

bool	Server::needsRegistration(const std::string &cmd)
{
	return (cmd != "PASS" && cmd != "NICK" && cmd != "USER" && cmd != "QUIT");
}

void	Server::parse_cmd(const std::string &cmd, int fd)
{
	Command	cmds = buildCommand(cmd);

	if (cmds.command.empty())
		return ;

	std::map<std::string, CmdHandler>::iterator it = _handlers.find(cmds.command);
	if (it == _handlers.end())
	{
		sendToClient(ERR_UNKNOWNCOMMAND(_clients[fd].getNick(), cmds.command), fd);
		return ;
	}

	if (needsRegistration(cmds.command)
		&& (!_clients[fd].isPass() || !_clients[fd].isRegistered()))
	{
		sendToClient(ERR_NOTREGISTERED(_clients[fd].getNick()), fd);
		return ;
	}

	(this->*(it->second))(cmds, fd);
}

/* ========================================================================== */
/*                              boucle principale                             */
/* ========================================================================== */

void	Server::acceptCline(int &lfd, std::vector<pollfd> &fds)
{
	sockaddr_in	clientAddr;
	socklen_t	clientAddrLen = sizeof(clientAddr);

	int incoming_call = accept(lfd, (struct sockaddr *)&clientAddr, &clientAddrLen);
	if (incoming_call < 0)
		return;

	int incoming_call_state = fcntl(incoming_call, F_SETFL, O_NONBLOCK);
	if (incoming_call_state < 0)
	{
		close(incoming_call);
		return;
	}

	pollfd lmo3idat;
	lmo3idat.fd = incoming_call;
	lmo3idat.events = POLLIN;
	lmo3idat.revents = 0;
	fds.push_back(lmo3idat);

	_clients.insert(std::make_pair(incoming_call, Client(incoming_call, inet_ntoa(clientAddr.sin_addr))));
}

void	Server::existingClient(int &fd, std::vector<pollfd> &fds, size_t &i)
{
	char	buffer[1024];
	ssize_t	size = recv(fd, buffer, sizeof(buffer) - 1, 0);

	if (size <= 0)
	{
		int dead_fd = fd;
		close(dead_fd);
		_clients.erase(dead_fd);
		fds.erase(fds.begin() + i);
		i--;
		return;
	}

	buffer[size] = '\0';
	Client &client = _clients[fd];
	client.appendToBuffer(buffer, size);
	client.print();

	std::vector<std::string> cmds = client.splitBuffer();
	for (size_t j = 0; j < cmds.size(); j++)
		this->parse_cmd(cmds[j], fd);
}

void	Server::Run()
{
	std::vector<pollfd>	fds;
	pollfd				lfd;

	lfd.fd = _lfd;
	lfd.events = POLLIN;
	lfd.revents = 0;
	fds.push_back(lfd);

	std::cout << "waiting clinet input" << std::endl;
	while (true)
	{
		for (size_t i = 0; i < fds.size(); i++)
		{
			if (fds[i].fd == _lfd)
				continue ;
			fds[i].events = POLLIN;
			std::map<int, Client>::iterator it = _clients.find(fds[i].fd);
			if (it != _clients.end() && it->second.hasPendingWrite())
				fds[i].events |= POLLOUT;
		}

		if (poll(fds.data(), fds.size(), -1) < 0)
		{
			if (errno == EINTR)
				continue;
			throw std::runtime_error("poll failed");
		}

		for (size_t i = 0; i < fds.size(); i++)
		{
			if (fds[i].revents & POLLOUT)
				flushClient(fds[i].fd);

			if (fds[i].revents & POLLIN)
			{
				if (fds[i].fd == _lfd)
					acceptCline(_lfd, fds);
				else
					existingClient(fds[i].fd, fds, i);
			}
		}
	}
}
