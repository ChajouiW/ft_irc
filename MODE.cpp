#include "Server.hpp"
#include "Command.hpp"

std::string	getMode(Channel &channel, int fd)
{
	std::ostringstream oss;
	std::string mode = "+", args = " ";
	if (channel.isInviteOnly())
		mode += "i";
	if (channel.isTopicRestricted())
		mode += "t";
	if (channel.hasKey())
	{
		mode += "k";
		args += channel.getKey() + " ";
	}
	if (channel.getLimit() > 0)
	{
		mode += "l";
		oss << channel.getLimit();
		args += oss.str();
	}
	if (channel.isInChannel(fd))
		mode += args;
	return mode;
}

void	changeModeKey(const Command &cmds, Server &server, int fd, size_t& index, bool adding, std::string& modeUpdates, std::string& modeArgs)
{
	if (adding)
	{
		if (cmds.args.size() < index + 1)
			return server.sendToClient(ERR_NEEDMOREPARAMS(server.getClient(fd).getNick(), "MODE"), fd);
		std::string key = cmds.args[index++];
		modeUpdates += "+k";
		modeArgs += key + " ";
		server.getChannel(cmds.args[0]).setKey(key);
	}
	else
	{
		modeUpdates += "-k";
		server.getChannel(cmds.args[0]).setKey("");
	}
}

void	changeModeLimit(const Command &cmds, Server &server, int fd, size_t& index,bool adding, std::string& modeUpdates, std::string& modeArgs)
{
	if (adding)
	{
		if (cmds.args.size() < index + 1)
			return server.sendToClient(ERR_NEEDMOREPARAMS(server.getClient(fd).getNick(), "MODE"), fd);
		modeUpdates += "+l";
		modeArgs += cmds.args[index] + " ";
		int limit = std::atoi(cmds.args[index++].c_str());
		server.getChannel(cmds.args[0]).setLimit(limit);
	}
	else
	{
		modeUpdates += "-l";
		server.getChannel(cmds.args[0]).setLimit(0);
	}
}

void	changeModeOperator(const Command &cmds, Server &server, int fd, size_t& index, bool adding, std::string& modeUpdates, std::string& modeArgs)
{
	if (cmds.args.size() < index + 1)
		return server.sendToClient(ERR_NEEDMOREPARAMS(server.getClient(fd).getNick(), "MODE"), fd);
	int targetFd = server.getClientfd(cmds.args[index++]);
	if (targetFd == -1)
		return server.sendToClient(ERR_NOSUCHNICK(server.getClient(fd).getNick(), cmds.args[index - 1]), fd);
	Channel &channel = server.getChannel(cmds.args[0]);
	if (!channel.isInChannel(targetFd))
		return server.sendToClient(ERR_USERNOTINCHANNEL(server.getClient(fd).getNick(), cmds.args[index - 1], channel.getName()), fd);
	if (adding)
	{
		modeUpdates += "+o";
		modeArgs += cmds.args[index - 1] + " ";
		channel.addOperator(targetFd);
	}
	else
	{
		modeUpdates += "-o";
		modeArgs += cmds.args[index - 1] + " ";
		channel.removeOperator(targetFd);
	}
}

void	Server::changeMode(const Command &cmds, int fd, std::string &modeUpdates)
{
	std::string modeChange = cmds.args[1], operation = "-+", modeArgs = " ";
	bool adding = (modeChange[0] != '-');
	Channel &channel = _channels[toUppercase(cmds.args[0])];
	size_t index = 2;
	for (size_t i = (modeChange[0] == '+' || modeChange[0] == '-'); i < modeChange.size(); ++i)
	{
		switch (modeChange[i])
		{
			case '+':
				adding = true;
				break;
			case '-':
				adding = false;
				break;
			case 'i':
				channel.setInviteOnly(adding);
				if (adding)
					modeUpdates += "+i";
				else
					modeUpdates += "-i";
				break;
			case 't':
				channel.setTopicRestricted(adding);
				if (adding)
					modeUpdates += "+t";
				else
					modeUpdates += "-t";
				break;
			case 'k':
				changeModeKey(cmds, *this, fd, index, adding, modeUpdates, modeArgs);
				break;
			case 'l':
				changeModeLimit(cmds, *this, fd, index, adding, modeUpdates, modeArgs);
				break;
			case 'o':
				changeModeOperator(cmds, *this, fd, index, adding, modeUpdates, modeArgs);
				break;
			default:
				sendToClient(ERR_UNKNOWNMODE(_clients[fd].getNick(), std::string(1, modeChange[i])), fd);
				break;
		}
	}
	modeArgs.erase(modeArgs.length() - 1);
	if (!modeUpdates.empty())
		modeUpdates += modeArgs;
}

void	Server::mode(const Command &cmds, int fd)
{
	if (cmds.args.empty())
		return sendToClient(ERR_NEEDMOREPARAMS(_clients[fd].getNick(), "MODE"), fd);
	std::map<std::string, Channel>::iterator it = _channels.find(toUppercase(cmds.args[0]));
	if (it == _channels.end())
		return sendToClient(ERR_NOSUCHCHANNEL(_clients[fd].getNick(), cmds.args[0]), fd);
	if (cmds.args.size() == 1)
		return sendToClient(RPL_CHANNELMODEIS(_clients[fd].getNick(), it->second.getName(), getMode(it->second, fd)), fd);

	Channel &channel = it->second;
	if (!channel.isInChannel(fd))
		return sendToClient(ERR_NOTONCHANNEL(_clients[fd].getNick(), channel.getName()), fd);
	if (!channel.isOperator(fd))
		return sendToClient(ERR_CHANOPRIVSNEEDED(_clients[fd].getNick(), channel.getName()), fd);
	std::string modeUpdates;
	changeMode(cmds, fd, modeUpdates);
	if (!modeUpdates.empty())
		broadcastMessage(it->first,_clients[fd].getPrefix() + " MODE " + channel.getName() + " " + modeUpdates + "\r\n");
}