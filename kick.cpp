#include "Server.hpp"

void    Server::kick(const Command& cmds, int fd)
{
    if (cmds.args.size() < 2)
        {sendToClient(ERR_NEEDMOREPARAMS(_clients[fd].getNick(), "KICK"), fd); return;}
    std::map<std::string, Channel>::iterator it = _channels.find(toUppercase(cmds.args[0]));
    if (it == _channels.end())
        {sendToClient(ERR_NOSUCHCHANNEL(_clients[fd].getNick(), cmds.args[0]), fd); return;}
    Channel& channel = it->second;
    if (!channel.isInChannel(fd))
        {sendToClient(ERR_NOTONCHANNEL(_clients[fd].getNick(), channel.getName()), fd); return;}
    if (!channel.isOperator(fd))
        {sendToClient(ERR_CHANOPRIVSNEEDED(_clients[fd].getNick(), channel.getName()), fd); return;}
    int targetFd = getClientfd(cmds.args[1]);
    if (targetFd == -1 || !channel.isInChannel(targetFd))
        {sendToClient(ERR_USERNOTINCHANNEL(_clients[fd].getNick(), cmds.args[1], channel.getName()), fd); return;}
    if (targetFd == fd)
        {sendToClient(":ircserv NOTICE " + _clients[fd].getNick() + " :You cannot kick yourself" + "\r\n", fd); return;}
    std::string reason;
    if (cmds.args.size() >= 3)
        joinMessage(reason, cmds.args, 2);
    std::string message = _clients[fd].getPrefix() + " KICK " + channel.getName() + " " + _clients[targetFd].getNick();
    if (!reason.empty())
        message += " :" + reason + "\r\n";
    else
        message += " :" + _clients[fd].getNick() + "\r\n";
    broadcastMessage(it->first, message);
    channel.removeMember(targetFd);
    channel.cancelInvits(targetFd);
}