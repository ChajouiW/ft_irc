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
		return sendToClient(ERR_NEEDMOREPARAMS(_clients[fd].getNick(), "TOPIC"), fd);

	std::map<std::string, Channel>::iterator it = _channels.find(toUppercase(cmd.args[0]));

	if (it == _channels.end())
		return sendToClient(ERR_NOSUCHCHANNEL(_clients[fd].getNick(), cmd.args[0]), fd);
	Channel& channel = it->second;
	if (!channel.isInChannel(fd))
		return sendToClient(ERR_NOTONCHANNEL(_clients[fd].getNick(), channel.getName()), fd);
	if (cmd.args.size() == 1 && !cmd.hasTrailing)
	{
		if (channel.getTopic().empty())
			return sendToClient(RPL_NOTOPIC(_clients[fd].getNick(), channel.getName()), fd);
		else
			return sendToClient(RPL_TOPIC(_clients[fd].getNick(), channel.getName(), channel.getTopic()), fd);
	}
	if (channel.isTopicRestricted() && !channel.isOperator(fd))
		return sendToClient(ERR_CHANOPRIVSNEEDED(_clients[fd].getNick(), channel.getName()), fd);
	changeTopic(cmd, fd, channel, it->first);
}