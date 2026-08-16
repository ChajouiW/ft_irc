#include "Server.hpp"

/* ========================================================================== */
/*                              registration                                  */
/* ========================================================================== */

void	Server::registerClient(int fd)
{
	if (_clients[fd].isPass() && !_clients[fd].getNick().empty() && !_clients[fd].getUser().empty())
	{
		_clients[fd].setRegistered(true);
		sendToClient(RPL_WELCOME(_clients[fd].getNick(), _clients[fd].getUser(), _clients[fd].getIp()), fd);
	}
}

/* ========================================================================== */
/*                                  PASS                                      */
/* ========================================================================== */

void	Server::checkPass(const Command &cmds, int fd)
{
	if (cmds.args.size() < 1)
	{
		sendToClient(ERR_NEEDMOREPARAMS(_clients[fd].getNick(), "PASS"), fd);
		return;
	}
	if (_clients[fd].isPass())
	{
		sendToClient(ERR_ALREADYREGISTERED(_clients[fd].getNick()), fd);
		return;
	}
	if (cmds.args[0] == _password)
		_clients[fd].setPass(true);
	else
		sendToClient(ERR_PASSWDMISMATCH(_clients[fd].getNick()), fd);
}

/* ========================================================================== */
/*                                  NICK                                      */
/* ========================================================================== */

bool	isValidNickname(const std::string &nick)
{
	if (nick.empty() || nick[0] == ':' || nick[0] == '#' || nick[0] == '&')
		return false;
	for (size_t i = 0; i < nick.size(); i++)
	{
		if (!std::isalnum(nick[i]) &&  nick[i] != '_')
			return false;
	}
	return true;
}

void	Server::setNickname(const Command &cmds, int fd)
{
	if (cmds.args.size() < 1)
	{
		sendToClient(ERR_NONICKNAMEGIVEN(_clients[fd].getNick()), fd);
		return;
	}
	if (!_clients[fd].isPass())
	{
		sendToClient(ERR_NOTREGISTERED(_clients[fd].getNick()), fd);
		return;
	}

	std::string nick = cmds.args[0];
	if (cmds.trailing.size() > 0)
		nick = cmds.trailing;

	if (!isValidNickname(nick))
	{
		sendToClient(ERR_ERRONEUSNICKNAME(_clients[fd].getNick(), nick), fd);
		return;
	}
	if (isInUse(nick))
	{
		sendToClient(ERR_NICKNAMEINUSE(_clients[fd].getNick(), nick), fd);
		return;
	}

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
		if (it->second.getNick() == nick)
			return true;
	}
	return false;
}

/* ========================================================================== */
/*                                  USER                                      */
/* ========================================================================== */

void	Server::setUsername(const Command &cmds, int fd)
{
	if (cmds.args.size() < 4)
	{
		sendToClient(ERR_NEEDMOREPARAMS(_clients[fd].getNick(), "USER"), fd);
		return;
	}
	if (!_clients[fd].isPass())
	{
		sendToClient(ERR_NOTREGISTERED(_clients[fd].getNick()), fd);
		return;
	}
	if (_clients[fd].isRegistered())
	{
		sendToClient(ERR_ALREADYREGISTERED(_clients[fd].getNick()), fd);
		return;
	}
	if (!_clients[fd].getUser().empty())
	{
		sendToClient(ERR_ALREADYREGISTERED(_clients[fd].getNick()), fd);
		return;
	}

	_clients[fd].setUser(cmds.args[0]);
	_clients[fd].setRealname(cmds.args[3]);
	if (cmds.trailing.size() > 0)
		_clients[fd].setRealname(cmds.trailing);
	registerClient(fd);
}

/* ========================================================================== */
/*                              QUIT / PING                                   */
/* ========================================================================== */

void	Server::disconnect(const Command &cmds, int fd)
{
	(void)cmds;
	(void)fd;
}

void	Server::ping(const Command &cmds, int fd)
{
	if (cmds.args.empty())
	{
		sendToClient(ERR_NOORIGIN(_clients[fd].getNick()), fd);
		return;
	}

	std::string response = ":" SERVER_NAME " PONG " SERVER_NAME " :" + cmds.args[0] + "\r\n";
	sendToClient(response, fd);
}
