#ifndef SERVER_HPP
#define SERVER_HPP
#include <cstddef>
#include <string>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <vector>
#include <iostream>
#include <sstream>
typedef struct pollfd pollfd;
#include "Client.hpp"
#include <map>
#include "Command.hpp"

// I WANT MAKE THE COMMAND AS KEY TO GET THERE METHODE FUNCTION LIKE MAP<"COMMAND", &FUNCTION> AND THEN I WILL CALL THE FUNCTION LIKE THIS map[command](args)

struct Command
{
	std::string command;
	std::vector<std::string> args;
	std::string trailing;
	Command() : command(""), args(std::vector<std::string>()), trailing("") {}
};


class Server
{
	public:
		// pointer-to-member: ma3ndhach this, 3lahaqach kandiro (this->*ptr)(...) f parse_cmd.
		// kola handler kayakhod jouj: line kamla o tokens. howa li ikhtar ach ista3mel,
		// o li ma7tajoch kayblank smiya dialo f definition (bla smiya = bla warning).
		typedef void (Server::*CmdHandler)(const Command &cmds, int fd);
	private:
		int _lfd;
		int _port;
		std::string _password;
		std::vector<int> _client_fds;
		std::map<int, Client> _clients;
		std::map<std::string, CmdHandler> _handlers;
		Server();
	public:
		Server(int	port, const std::string &pass): _lfd(-1), _port(port), _password(pass)
		{
			// & o Server:: jouj wajibin: smiya dial member ma katwalich pointer b rasha
			_handlers["PASS"] = &Server::checkPass;
			_handlers["NICK"] = &Server::setNickname;
			_handlers["USER"] = &Server::setUsername;
			_handlers["QUIT"] = &Server::disconnect;
			_handlers["PING"] = &Server::ping;
			// _handlers["KICK"] = &Server::kick;
			// _handlers["PRIVMSG"] = &Server::privmsg;
			// _handlers["JOIN"] = &Server::join;
			// _handlers["PART"] = &Server::part;
			// _handlers["TOPIC"] = &Server::topic;
			// _handlers["INVITE"] = &Server::invite;
			// _handlers["MODE"] = &Server::mode;
		};
		void	registerClient(int fd);
		bool	isInUse(const std::string &nick);
		std::vector<std::string> split_cmd(const std::string &cmd);

		void	parse_cmd(const std::string &cmd, int fd);

		void	checkPass(const Command &cmds, int fd);
		void	setNickname(const Command &cmds, int fd);
		void	setUsername(const Command &cmds, int fd);
		void	disconnect(const Command &cmds, int fd); // still khawya
		void	ping(const Command &cmds, int fd); // ri tkharbi9a ba9i ma3raftch chno khas dir
		// void	kick(const Command &cmds, int fd);
		// void	privmsg(const Command &cmds, int fd);
		// void	join(const Command &cmds, int fd);   // HADO BA9I MA KAMLOCH
		// void	part(const Command &cmds, int fd);
		// void	topic(const Command &cmds, int fd);
		// void	invite(const Command &cmds, int fd);
		// void	mode(const Command &cmds, int fd);

		void    sendToClient(const std::string &message, int fd);

		void	setup();
		/* struct pollfd 
               int   fd;        ///////file descriptor ///
               short events;    ///////requested events ///
               short revents;   ///////returned events ///
         */
		void	Run();
		void	acceptCline(int &lfd, std::vector<pollfd> &fds);
		void	existingClient(int &fd, std::vector<pollfd> &fds, size_t &i);
		~Server()
		{
			if (_lfd >= 0)
				close(_lfd);
		}
};
#endif