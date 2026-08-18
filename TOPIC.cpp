#include "Server.hpp"

void	Server::changeTopic(const Command& cmd, int fd, Channel& channel, const std::string& channelName)
{
	if (cmd.args.size() >= 2)
	{
		std::string newTopic;
		joinMessage(newTopic, cmd.args, 1);
		channel.setTopic(newTopic);
		std::string message = _clients[fd].getPrefix() + " TOPIC " + channel.getName() + " :" + newTopic + "\r\n";
		broadcastMessage(channelName, message);
	}
	else if (cmd.hasTrailing)
	{
		std::string newTopic = cmd.trailing;
		channel.setTopic(newTopic);
		std::string message = _clients[fd].getPrefix() + " TOPIC " + channel.getName() + " :" + newTopic + "\r\n";
		broadcastMessage(channelName, message);
	}
}
void	Server::topic(const Command& cmd, int fd)
{
	if (cmd.args.empty())
		{sendToClient(ERR_NEEDMOREPARAMS(_clients[fd].getNick(), "TOPIC"), fd); return;}

	std::map<std::string, Channel>::iterator it = _channels.find(toUppercase(cmd.args[0]));

	if (it == _channels.end())
	{
		sendToClient(ERR_NOSUCHCHANNEL(_clients[fd].getNick(), cmd.args[0]), fd);
		return;
	}
	Channel& channel = it->second;
	if (!channel.isInChannel(fd))
	{
		sendToClient(ERR_NOTONCHANNEL(_clients[fd].getNick(), channel.getName()), fd);
		return;
	}
	if (cmd.args.size() == 1 && !cmd.hasTrailing)
	{
		if (channel.getTopic().empty())
			sendToClient(RPL_NOTOPIC(_clients[fd].getNick(), channel.getName()), fd);
		else
			sendToClient(RPL_TOPIC(_clients[fd].getNick(), channel.getName(), channel.getTopic()), fd);
		return;
	}
	if (channel.isTopicRestricted() && !channel.isOperator(fd))
	{
		sendToClient(ERR_CHANOPRIVSNEEDED(_clients[fd].getNick(), channel.getName()), fd);
		return;
	}
	changeTopic(cmd, fd, channel, it->first);
}