#include "Server.hpp"

std::vector<std::string>	extractTarget(const std::string& target)
{
	std::vector<std::string> targets;
	std::istringstream stream(target);
	std::string token;
	while (std::getline(stream, token, ','))
	{
		if (!token.empty())
			targets.push_back(token);
	}
	return targets;
}

void	Server::sendToChannel(const std::string& channel, const std::string& message, int fd)
{
	std::map<std::string, Channel>::iterator it = _channels.find(toUppercase(channel));
	if (it == _channels.end())
	{
		sendToClient(ERR_NOSUCHCHANNEL(_clients[fd].getNick(), channel), fd);
		return;
	}
	if (!it->second.isInChannel(fd))
	{
		sendToClient(ERR_CANNOTSENDTOCHAN(_clients[fd].getNick(), channel), fd);
		return;
	}
	std::string fullMessage = _clients[fd].getPrefix() + " PRIVMSG " + it->second.getName() + " :" + message + "\r\n";
	broadcastMessage(it->first, fullMessage, fd);
}

void	Server::toClient(const std::string& target, const std::string& message, int fd)
{
	bool found = false;
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (toUppercase(it->second.getNick()) == toUppercase(target))
		{
			std::string fullMessage = it->second.getPrefix() + " PRIVMSG " + target + " :" + message + "\r\n";
			sendToClient(fullMessage, it->first);
			found = true;
			break;
		}
	}
	if (!found)
		sendToClient(ERR_NOSUCHNICK(_clients[fd].getNick(), target), fd);
}

void	joinMessage(std::string& message, const std::vector<std::string>& args, size_t startIndex)
{
	for (size_t i = startIndex; i < args.size(); ++i)
		message += args[i] + " ";
	if (!message.empty() && message[message.size() - 1] == ' ')
		message.erase(message.size() - 1);
}

void	Server::privMsg(const Command &cmds, int fd)
{
	if (cmds.args.size() < 1)
		{sendToClient(ERR_NORECIPIENT(_clients[fd].getNick(), "PRIVMSG"), fd); return;}
	if (cmds.args.size() < 2)
		{sendToClient(ERR_NOTEXTTOSEND(_clients[fd].getNick()), fd); return;}

	std::vector<std::string> target = extractTarget(cmds.args[0]);
	std::string message;
	if (cmds.args.size() >= 2)
		joinMessage(message, cmds.args, 1);

	for (size_t i = 0; i < target.size(); ++i)
	{
		if (target[i][0] == '#' || target[i][0] == '&')
			sendToChannel(target[i], message, fd);
		else
			toClient(target[i], message, fd);
	}
}