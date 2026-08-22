#include "Server.hpp"

void	Server::invite(const Command& cmds, int fd)
{
	if (cmds.args.size() < 2)
		return sendToClient(ERR_NEEDMOREPARAMS(_clients[fd].getNick(), "INVITE"), fd);

	std::string invitedName = cmds.args[0];
	std::string channelName = cmds.args[1];
	int invitedFd = getClientfd(invitedName);
	if (invitedFd == -1)
		return sendToClient(ERR_NOSUCHNICK(_clients[fd].getNick(), invitedName), fd);

	std::map<std::string, Channel>::iterator it = _channels.find(toUppercase(channelName));
	if (it == _channels.end())
		return sendToClient(ERR_NOSUCHCHANNEL(_clients[fd].getNick(), channelName), fd);

	Channel&	channel = it->second;
	if (!channel.isInChannel(fd))
		return sendToClient(ERR_NOTONCHANNEL(_clients[fd].getNick(), channelName), fd);
	if (channel.isInviteOnly() && !channel.isOperator(fd))
		return sendToClient(ERR_CHANOPRIVSNEEDED(_clients[fd].getNick(), channel.getName()), fd);
	if (channel.isInChannel(invitedFd))
		return sendToClient(ERR_USERONCHANNEL(_clients[fd].getNick(), invitedName, channelName), fd);

	channel.addInvited(invitedFd);
	sendToClient(RPL_INVITING(_clients[fd].getNick(), invitedName, channel.getName()), fd);
	std::string msg = _clients[fd].getPrefix() + " INVITE " + invitedName + " " + channel.getName() + "\r\n";
	sendToClient(msg, invitedFd);
}