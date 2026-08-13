#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <poll.h>

typedef struct pollfd pollfd;

#include "Client.hpp"
#include "Command.hpp"

struct Command
{
	std::string					command;
	std::vector<std::string>	args;
	std::string					trailing;

	Command() : command(""), args(std::vector<std::string>()), trailing("") {}
};

class Server
{
	public:
		typedef void (Server::*CmdHandler)(const Command &cmds, int fd);

	private:
		int									_lfd;
		int									_port;
		std::string							_password;
		std::vector<int>					_client_fds;
		std::map<int, Client>				_clients;
		std::map<std::string, CmdHandler>	_handlers;

		Server();

	public:
		Server(int port, const std::string &pass);
		~Server();

		void	setup();
		void	Run();
		void	acceptCline(int &lfd, std::vector<pollfd> &fds);
		void	existingClient(int &fd, std::vector<pollfd> &fds, size_t &i);

		void	sendToClient(const std::string &message, int fd);
		void	flushClient(int fd);

		std::vector<std::string>	split_cmd(const std::string &cmd);
		Command						buildCommand(const std::string &line);
		bool						needsRegistration(const std::string &cmd);
		void						parse_cmd(const std::string &cmd, int fd);

		void	checkPass(const Command &cmds, int fd);
		void	setNickname(const Command &cmds, int fd);
		void	setUsername(const Command &cmds, int fd);
		void	disconnect(const Command &cmds, int fd);
		void	ping(const Command &cmds, int fd);

		void	registerClient(int fd);
		bool	isInUse(const std::string &nick);
};

#endif
