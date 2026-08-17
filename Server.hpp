#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <poll.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct pollfd pollfd;

#include "Client.hpp"
#include "Channel.hpp"
#include "Command.hpp"

struct Command
{
	std::string					command;
	std::vector<std::string>	args;
	std::string					trailing;

	Command() : command(""), args(std::vector<std::string>()), trailing("") {}
};

struct	findPollFd
{
	int	fd;
	findPollFd(int fd) : fd(fd) {}
	bool	operator()(const pollfd& pfd)
	{
		return pfd.fd == fd;
	}
};

class Server
{
	public:
		typedef void (Server::*CmdHandler)(const Command &cmds, int fd);

	private:
		int									_lfd;
		int									_port;
		std::string							_password;
		std::vector<pollfd>					_fds;
		// std::vector<int>					_client_fds;
		std::map<int, Client>				_clients;
		std::map<std::string, CmdHandler>	_handlers;
		std::map<std::string, Channel>		_channels;

		Server();

	public:
		Server(int port, const std::string &pass);
		~Server();

		void	setup();
		void	Run();

		void	acceptCline(int &lfd);
		void	existingClient(int fd);
		void	joinAlert(const Channel &channel, int fd);
		void	broadcastMessage(const std::string& channelName, const std::string &message);
		void	broadcastMessage(const std::string& channelName, const std::string &message, int fd);
		const	std::string getMembersList(std::string channelName);

		void	sendToClient(const std::string &message, int fd);
		void	flushClient(int fd);

		std::vector<std::string>	split_cmd(const std::string &cmd);
		Command						buildCommand(const std::string &line);
		bool						needsRegistration(const std::string &cmd);
		void						parse_cmd(const std::string &cmd, int fd);

		void	removeClientFromChannels(int fd, const std::string &quitMessage);

		void	checkPass(const Command &cmds, int fd);
		void	setNickname(const Command &cmds, int fd);
		void	setUsername(const Command &cmds, int fd);
		void	disconnect(int fd, const std::string &quitMessage);
		void	quit(const Command &cmds, int fd);
		void	joinChannel(const Command &cmds, int fd);
		void	privMsg(const Command &cmds, int fd);
		void	kick(const Command &cmds, int fd);
		void	ping(const Command &cmds, int fd);

		void	sendToChannel(const std::string& channel, const std::string& message, int fd);
		void	toClient(const std::string& target, const std::string& message, int fd);

		void	existingChannel(std::map<std::string, Channel>::iterator it, const std::string &key, int fd);
		void	newChannel(const std::string &channelName, const std::string &key, int fd);
		void	registerClient(int fd);
		bool	isInUse(const std::string &nick);
		int		getClientfd(const std::string &nick);
};
std::string	toUppercase(std::string str);
void	joinMessage(std::string& message, const std::vector<std::string>& args, size_t startIndex);

#endif
