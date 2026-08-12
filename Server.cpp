#include "Server.hpp"

#include <cctype>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct pollfd pollfd;

Server::Server(int	port, const std::string &pass): _lfd(-1), _port(port), _password(pass){}

Server::~Server()
{
	if (_lfd >= 0)
		close(_lfd);
}

std::string	Server::toUpper(const std::string &s)
{
	std::string	out = s;

	for (size_t i = 0; i < out.size(); i++)
		out[i] = std::toupper(static_cast<unsigned char>(out[i]));
	return out;
}

void Server::parse_args(t_command &cmd, const std::string &rest)
{
	size_t start = rest.find_first_not_of(' ');
	if (start == std::string::npos)
		return;
	while (start < rest.size())
	{
		if (rest[start] == ':')
		{
			cmd.trailing = rest.substr(start + 1);
			break;
		}
		size_t space_pos = rest.find(' ', start);
		if (space_pos == std::string::npos)
		{
			cmd.params.push_back(rest.substr(start));
			break;
		}
		cmd.params.push_back(rest.substr(start, space_pos - start));
		start = rest.find_first_not_of(' ', space_pos);
		if (start == std::string::npos)
			break;
	}
	if (!cmd.trailing.empty())
		cmd.params.push_back(cmd.trailing);
}

void Server::parse_command(const std::string &line, t_command &cmd, std::string &rest)
{
	size_t start = line.find_first_not_of(" \t");
	if (start == std::string::npos)
		return;
	// Find the command
	size_t space_pos = line.find(' ', start);
	if (space_pos == std::string::npos)
	{
		cmd.command = line.substr(start);
		cmd.command = toUpper(cmd.command);
		return;
	}
	std::string command = line.substr(start, space_pos - start);
	cmd.command = command;
	cmd.command = toUpper(cmd.command);
	rest = line.substr(space_pos + 1);
	// std::cout << rest << std::endl;
}

void Server::printf_command(const t_command &cmd)
{

	std::cout << "Command: " << cmd.command << std::endl;
	std::cout << "Params: ";
	for (size_t i = 0; i < cmd.params.size(); ++i)
	{
		std::cout << cmd.params[i];
		if (i < cmd.params.size() - 1)
			std::cout << ", ";
	}
	std::cout << std::endl;
	std::cout << "Trailing: " << cmd.trailing << std::endl;
}

void Server::dispatcher(const t_command &cmd, int fd)
{


	// Parse the command using the parser.hpp structure
	// This is a placeholder for actual command parsing logic
	std::string rest; // parse args based on the command structure
	(void)fd; // Suppress unused variable warning
	printf_command(cmd);
	if (cmd.command.empty())
		return;
	if (cmd.command == "PASS")
	{
		if (cmd.params.empty())
		{
			std::cerr << "Error: PASS command requires a parameter" << std::endl;
			return;
		}
	}
	else if (cmd.command == "NICK")
	{
		if (cmd.params.empty())
		{
			std::cerr << "Error: NICK command requires a parameter" << std::endl;
			return;
		}
	}
	else if (cmd.command == "USER")
	{
		if (cmd.params.size() < 4)
		{
			std::cerr << "Error: USER command requires 4 parameters" << std::endl;
			return;
		}
	}
	else if (cmd.command == "QUIT")
	{
		std::cout << "Client " << fd << " disconnected." << std::endl;
		close(fd);
		_clients.erase(fd);
		return;
	}
	else if (cmd.command == "PRIVMSG")
	{
		if (cmd.params.size() < 2)
		{
			std::cerr << "Error: PRIVMSG command requires a target and a message" << std::endl;
			return;
		}
		std::string target = cmd.params[0];
		std::string message = cmd.trailing;
		std::cout << "Sending message to " << target << ": " << message << std::endl;
		// Here you would implement the logic to send the message to the target client(s)
	}
	else if (cmd.command == "JOIN")
	{
		if (cmd.params.empty())
		{
			std::cerr << "Error: JOIN command requires a channel parameter" << std::endl;
			return;
		}
		std::string channel = cmd.params[0];
		std::cout << "Client " << fd << " joining channel: " << channel << std::endl;
		// Here you would implement the logic to add the client to the specified channel
	}
	else if (cmd.command == "PART")
	{
		if (cmd.params.empty())
		{
			std::cerr << "Error: PART command requires a channel parameter" << std::endl;
			return;
		}
		std::string channel = cmd.params[0];
		std::cout << "Client " << fd << " leaving channel: " << channel << std::endl;
		// Here you would implement the logic to remove the client from the specified channel
	}
	else if(cmd.command == "PING")
	{
		std::cout << "Received PING from client " << fd << std::endl;
		// Here you would implement the logic to respond with a PONG message
	}
	else if(cmd.command == "PONG")
	{
		std::cout << "Received PONG from client " << fd << std::endl;
		// Here you would implement the logic to handle the PONG response
	}
	else if (cmd.command == "MODE")
	{
		if (cmd.params.empty())
		{
			std::cerr << "Error: MODE command requires a target parameter" << std::endl;
			return;
		}
		std::string target = cmd.params[0];
		std::cout << "Client " << fd << " changing mode for: " << target << std::endl;
		// Here you would implement the logic to change the mode for the specified target
	}
	else if (cmd.command == "TOPIC")
	{
		if (cmd.params.empty())
		{
			std::cerr << "Error: TOPIC command requires a channel parameter" << std::endl;
			return;
		}
		std::string channel = cmd.params[0];
		std::cout << "Client " << fd << " setting topic for channel: " << channel << std::endl;
		// Here you would implement the logic to set the topic for the specified channel
	}
	else
	{
		std::cerr << "Error: Unknown command: " << cmd.command << std::endl;
		return;
	}


}

void	Server::setup()
{
	sockaddr_in s;
	memset(&s, 0, sizeof(s));
	s.sin_family = AF_INET;
	s.sin_addr.s_addr = INADDR_ANY;
	s.sin_port = htons(_port);
	_lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (_lfd < 0)
		throw std::runtime_error("Socket creation failed");
	int opt = 1;
	if (setsockopt(_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw std::runtime_error("setsockopt failed");
	if (bind(_lfd, (struct sockaddr *)&s, sizeof(s)) < 0)
		throw std::runtime_error("Bind failed");
	if (listen(_lfd, 10) < 0)
		throw std::runtime_error("Listen failed");
	if (fcntl(_lfd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl failed");
}

/* struct pollfd
               int   fd;        ///////file descriptor ///
               short events;    ///////requested events ///
               short revents;   ///////returned events ///
         */
void Server::Run()
{

	std::vector<pollfd> fds;
	pollfd lfd;
	lfd.fd = _lfd;
	lfd.events = POLLIN;
	lfd.revents = 0;
	fds.push_back(lfd);

	while (true)
	{
		std::cout << "waiting clinet input" << std::endl;
		if (poll(fds.data(), fds.size(), -1) < 0)
		{
			if (errno == EINTR)
				continue;
			throw std::runtime_error("poll failed");
		}
		for (size_t i = 0; i < fds.size();i++)
		{
			if (fds[i].revents & POLLIN)
			{
				if (fds[i].fd == _lfd)
				{
					int incoming_call = accept(_lfd, NULL, NULL);
					if (incoming_call < 0)
						continue;
					int incoming_call_state = fcntl(incoming_call, F_SETFL,O_NONBLOCK); // non block kat3ni anaha lmain thread matb9ach tsna taidir lfd lakhor chi input b7al kima kna kandiro f minishell cat katsna input hadi la kadesactiviha
					if (incoming_call_state < 0)												// F_SETF == File discriptor SET FL.AG
					{
						close(incoming_call);
						continue; // ma3ndkch mo3idat azpi skipi talfo9 again;!!!!
					}
					pollfd lmo3idat; // hna kadir lmo3idat lljondi  jdid okat7to ftiara dial free fire
					lmo3idat.fd = incoming_call;
					lmo3idat.events = POLLIN;
					lmo3idat.revents = 0;
					fds.push_back(lmo3idat);
					_clients.insert(std::make_pair(incoming_call, Client(incoming_call))); // hna katpushi lclient jdid fmap dial clients li kayn f server

				}
				else // hna katchecki mn b3d okatla9a bl jondi li deja pushitih fliteration lwla okat9ol lih yala tla7 ojib lia
				{
						char buffer [1024];
						ssize_t size = recv(fds[i].fd,buffer,sizeof(buffer) - 1,0);
						if (size <= 0)
						{
							// 0 sdha mn raso or client deconecta saf
							// < 0 rah kain issue f socket handlih t7wa
							int dead_fd = fds[i].fd; // khodo 9bel materasi, mn b3d lerase fds[i] rah wa7d akhor
							close(dead_fd);
							_clients.erase(dead_fd);
							fds.erase(fds.begin() + i);
							i--;
							continue;
						}
						buffer[size] = '\0';
						Client &client = _clients[fds[i].fd];
						client.appendToBuffer(buffer, size);
						// client.print();
						std::string line;
						while (client.extractCommand(line))
						{
							t_command cmd;
							std::string rest;
							parse_command(line, cmd, rest);
							parse_args(cmd, rest);
							std::cout << "Received command from client " << fds[i].fd << ": " << line << std::endl;
							dispatcher(cmd, fds[i].fd); // hna kat9ra lcommand li jiti mn client okat7to fbuffer dialo
							if (cmd.command == "Quit")
							{
								i--;
								break;
							}

							// std::cout << "Received command from client " << fds[i].fd << ": " << line << std::endl;
						}
						// std::vector<std::string> cmds = client.splitBuffer(); // hna kat9ra lcommand li jiti mn client okat7to fbuffer dialo
						// for (size_t j = 0; j < cmds.size(); j++)
						// 	this->parse_cmd(cmds[j], fds[i].fd); // hna katparse lcommand li jiti mn client okat9raha ffunction parse_cmd li kayn f server.cpp

				}
			}
		}

	}
}
