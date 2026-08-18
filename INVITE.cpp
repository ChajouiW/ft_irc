#include "Server.hpp"

void	Server::invite(const Command& cmds, int fd)
{
	if (cmds.args.size() < 2)
		{sendToClient(ERR_NEEDMOREPARAMS(_clients[fd].getNick(), "INVITE"), fd); return ;}

	std::string invitedName = cmds.args[0];
	std::string channelName = cmds.args[1];
	int invitedFd = getClientfd(invitedName);
	if (invitedFd == -1)
		{sendToClient(ERR_NOSUCHNICK(_clients[fd].getNick(), invitedName), fd); return ;}

	std::map<std::string, Channel>::iterator it = _channels.find(toUppercase(channelName));
	if (it == _channels.end())
		{sendToClient(ERR_NOSUCHCHANNEL(_clients[fd].getNick(), channelName), fd); return ;}

	Channel&	channel = it->second;
	if (!channel.isInChannel(fd))
		{sendToClient(ERR_NOTONCHANNEL(_clients[fd].getNick(), channelName), fd); return ;}
	if (channel.isInviteOnly() && !channel.isOperator(fd))
		{sendToClient(ERR_CHANOPRIVSNEEDED(_clients[fd].getNick(), channel.getName()), fd); return;}
	if (channel.isInChannel(invitedFd))
		{sendToClient(ERR_USERONCHANNEL(_clients[fd].getNick(), invitedName, channelName), fd); return;}

	channel.addInvited(invitedFd);
	sendToClient(RPL_INVITING(_clients[fd].getNick(), invitedName, channel.getName()), fd);
	std::string msg = _clients[fd].getPrefix() + " INVITE " + invitedName + " " + channel.getName() + "\r\n";
	sendToClient(msg, invitedFd);
}