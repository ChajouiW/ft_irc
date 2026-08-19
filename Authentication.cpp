#include "Server.hpp"

/* ========================================================================== */
/*				registration								  */
/* ========================================================================== */

void	Server::registerClient(int fd)
{
	if (_clients[fd].isPass() && _clients[fd].getNick() != "*" && !_clients[fd].getUser().empty())
	{
		_clients[fd].setRegistered(true);
		sendToClient(RPL_WELCOME(_clients[fd].getNick(), _clients[fd].getUser(), _clients[fd].getIp()), fd);
	}
}

/* ========================================================================== */
/*				  PASS									  */
/* ========================================================================== */

void	Server::checkPass(const Command &cmds, int fd)
{
	if (cmds.args.size() < 1)
		return sendToClient(ERR_NEEDMOREPARAMS(_clients[fd].getNick(), "PASS"), fd);
	if (_clients[fd].isPass())
		return sendToClient(ERR_ALREADYREGISTERED(_clients[fd].getNick()), fd);
	if (cmds.args[0] == _password)
		_clients[fd].setPass(true);
	else
		sendToClient(ERR_PASSWDMISMATCH(_clients[fd].getNick()), fd);
}

/* ========================================================================== */
/*				  NICK									  */
/* ========================================================================== */

bool	isValidNickname(const std::string &nick)
{
	if (nick.empty() || nick[0] == ':' || nick[0] == '#' || nick[0] == '&')
		return false;
	for (size_t i = 0; i < nick.size(); i++)
	{
		if (!std::isalnum(static_cast<unsigned char>(nick[i])) &&  nick[i] != '_')
			return false;
	}
	return true;
}

void	Server::setNickname(const Command &cmds, int fd)
{
	if (cmds.args.size() < 1)
		return sendToClient(ERR_NONICKNAMEGIVEN(_clients[fd].getNick()), fd);
	if (!_clients[fd].isPass())
		return sendToClient(ERR_NOTREGISTERED(_clients[fd].getNick()), fd);

	std::string nick = cmds.args[0];
	if (cmds.trailing.size() > 0)
		nick = cmds.trailing;

	if (!isValidNickname(nick))
		return sendToClient(ERR_ERRONEUSNICKNAME(_clients[fd].getNick(), nick), fd);
	if (isInUse(nick))
		return sendToClient(ERR_NICKNAMEINUSE(_clients[fd].getNick(), nick), fd);

	if (_clients[fd].isRegistered())
	{
		std::string oldNick = _clients[fd].getNick();
		std::string response = "Nickname changed from " + oldNick + " to " + nick + ".\r\n";
		sendToClient(response, fd);
		_clients[fd].setNick(nick);
		return;
	}

	_clients[fd].setNick(nick);
	registerClient(fd);
}

bool	Server::isInUse(const std::string &nick)
{
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (toUppercase(it->second.getNick()) == toUppercase(nick))
			return true;
	}
	return false;
}

/* ========================================================================== */
/*				  USER									  */
/* ========================================================================== */

void	Server::setUsername(const Command &cmds, int fd)
{
	if (cmds.args.size() < 4)
		return sendToClient(ERR_NEEDMOREPARAMS(_clients[fd].getNick(), "USER"), fd);
	if (!_clients[fd].isPass())
		return sendToClient(ERR_NOTREGISTERED(_clients[fd].getNick()), fd);
	if (_clients[fd].isRegistered())
		return sendToClient(ERR_ALREADYREGISTERED(_clients[fd].getNick()), fd);
	if (!_clients[fd].getUser().empty())
		return sendToClient(ERR_ALREADYREGISTERED(_clients[fd].getNick()), fd);

	_clients[fd].setUser(cmds.args[0]);
	_clients[fd].setRealname(cmds.args[3]);
	if (cmds.trailing.size() > 0)
		_clients[fd].setRealname(cmds.trailing);
	registerClient(fd);
}

/* ========================================================================== */
/*				QUIT / PING		   */
/* ========================================================================== */

void	Server::removeClientFromChannels(int fd, const std::string &quitMessage)
{
	for (std::map<std::string, Channel>::iterator it = _channels.begin(); it != _channels.end();)
	{
		std::map<std::string, Channel>::iterator tmp = it++;
		tmp->second.cancelInvits(fd);
		if (tmp->second.isInChannel(fd))
		{
			broadcastMessage(tmp->first, _clients[fd].getPrefix() + " QUIT :" + quitMessage + "\r\n", fd);
			tmp->second.removeMember(fd);
			if (tmp->second.getMembers().empty() && tmp->second.getOperators().empty())
				_channels.erase(tmp);
		}
	}
}

void	Server::disconnect(int fd, const std::string &quitMessage)
{
	if (_clients.find(fd) == _clients.end())
		return;
	std::cout << "Client disconnected: " << _clients[fd].getFd() << std::endl;
	std::vector<pollfd>::iterator it = std::find_if(_fds.begin(), _fds.end(), findPollFd(fd));
	if (it != _fds.end())
		_fds.erase(it);
	removeClientFromChannels(fd, quitMessage);
	_clients.erase(fd);
	close(fd);
}

void	Server::quit(const Command &cmds, int fd)
{
	std::string quitMessage = "Client Quit";
	if (!cmds.trailing.empty())
		quitMessage = cmds.trailing;
	else if (!cmds.args.empty())
		quitMessage = cmds.args[0];

	disconnect(fd, quitMessage);
}

void	Server::ping(const Command &cmds, int fd)
{
	if (cmds.args.empty())
		return sendToClient(ERR_NOORIGIN(_clients[fd].getNick()), fd);

	std::string response = ":" SERVER_NAME " PONG " SERVER_NAME " :" + cmds.args[0] + "\r\n";
	sendToClient(response, fd);
}
