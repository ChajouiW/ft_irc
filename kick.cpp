#include "Server.hpp"

void    Server::kick(const Command& cmds, int fd)
{
    if (cmds.args.size() < 2)
        return sendToClient(ERR_NEEDMOREPARAMS(_clients[fd].getNick(), "KICK"), fd);
    std::map<std::string, Channel>::iterator it = _channels.find(toUppercase(cmds.args[0]));
    if (it == _channels.end())
        return sendToClient(ERR_NOSUCHCHANNEL(_clients[fd].getNick(), cmds.args[0]), fd);
    Channel& channel = it->second;
    if (!channel.isInChannel(fd))
        return sendToClient(ERR_NOTONCHANNEL(_clients[fd].getNick(), channel.getName()), fd);
    if (!channel.isOperator(fd))
        return sendToClient(ERR_CHANOPRIVSNEEDED(_clients[fd].getNick(), channel.getName()), fd);
    int targetFd = getClientfd(cmds.args[1]);
    if (targetFd == -1 || !channel.isInChannel(targetFd))
        return sendToClient(ERR_USERNOTINCHANNEL(_clients[fd].getNick(), cmds.args[1], channel.getName()), fd);
    if (targetFd == fd)
        return sendToClient(":ircserv NOTICE " + _clients[fd].getNick() + " :You cannot kick yourself" + "\r\n", fd);
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