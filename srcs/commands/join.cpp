#include "Server.hpp"
#include "Command.hpp"
#include <utility>
#include <sstream>

void	Server::joinAlert(const Channel &newChannel, int fd)
{
	broadcastMessage(toUppercase(newChannel.getName()), _clients[fd].getPrefix() + " JOIN " + newChannel.getName() + "\r\n");
	if (newChannel.getTopic().empty())
		sendToClient(RPL_NOTOPIC(_clients[fd].getNick(), newChannel.getName()), fd);
	else
		sendToClient(RPL_TOPIC(_clients[fd].getNick(), newChannel.getName(), newChannel.getTopic()), fd);
	sendToClient(RPL_NAMREPLY(_clients[fd].getNick(), newChannel.getName(), getMembersList(newChannel.getName())), fd);
	sendToClient(RPL_ENDOFNAMES(_clients[fd].getNick(), newChannel.getName()), fd);
}

void	Server::newChannel(const std::string &channelName, const std::string &key, int fd)
{
	Channel newChannel(channelName, key);
	newChannel.addOperator(fd);
	_channels[toUppercase(channelName)] = newChannel;
	// sendToClient(_clients[fd].getPrefix() + " JOIN " + channelName + "\r\n", fd);
	joinAlert(newChannel, fd);
}

void	Server::existingChannel(std::map<std::string, Channel>::iterator it, const std::string &key, int fd)
{
	if (it->second.isMember(fd) || it->second.isOperator(fd))
		return sendToClient(RPL_ALREADYJOINED(_clients[fd].getNick(), it->second.getName()), fd);
	if (it->second.isInviteOnly() && !it->second.isInvited(fd))
		return sendToClient(ERR_INVITEONLYCHAN(_clients[fd].getNick(), it->second.getName()), fd);
	if (it->second.hasKey() && it->second.getKey() != key && !it->second.isInvited(fd))
		return sendToClient(ERR_BADCHANNELKEY(_clients[fd].getNick(), it->second.getName()), fd);
	if (it->second.getLimit() > 0 && it->second.getChannelSize() >= it->second.getLimit())
		return sendToClient(ERR_CHANNELISFULL(_clients[fd].getNick(), it->second.getName()), fd);
	it->second.addMember(fd);
	if (it->second.isInvited(fd))
		it->second.removeInvited(fd);
	joinAlert(it->second, fd);
}

std::vector<std::pair<std::string, std::string> > extractChannelsInfo(const std::vector<std::string>& args)
{
	std::vector<std::pair<std::string, std::string> > channels;
	std::vector<std::string> channelNames, keys;
	std::istringstream stream1(args[0]), stream2("");
	if (args.size() > 1)
		stream2.str(args[1]);
	std::string token;
	while (std::getline(stream1, token, ','))
	{
		if (!token.empty())
			channelNames.push_back(token);
	}
	while (std::getline(stream2, token, ','))
		keys.push_back(token);
	for (size_t i = 0; i < channelNames.size(); ++i)
	{
		if (i < keys.size())
			channels.push_back(std::make_pair(channelNames[i], keys[i]));
		else
			channels.push_back(std::make_pair(channelNames[i], ""));
	}
	return channels;
}

void	Server::joinChannel(const Command &cmds, int fd)
{
	if (cmds.args.empty())
		return sendToClient(ERR_NEEDMOREPARAMS(_clients[fd].getNick(), "JOIN"), fd);
	std::vector<std::pair<std::string, std::string> > channeNames = extractChannelsInfo(cmds.args);
	for (size_t i = 0; i < channeNames.size(); ++i)
	{
		std::string &channelName = channeNames[i].first;
		std::string &key = channeNames[i].second;
		if (channelName.size() < 2 || (channelName[0] != '#' && channelName[0] != '&') || channelName.find(' ') != std::string::npos)
		{
			sendToClient(ERR_NOSUCHCHANNEL(_clients[fd].getNick(), channelName), fd);
			continue;
		}

		std::map<std::string, Channel>::iterator it = _channels.find(toUppercase(channelName));
		if (it != _channels.end())
			existingChannel(it, key, fd);
		else
			newChannel(channelName, key, fd);
	}
}