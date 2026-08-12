#ifndef SERVER_HPP
#define SERVER_HPP
#include <cstddef>
#include <string>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
// #include <string.h>
#include <cstring>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <vector>
#include <iostream>
typedef struct pollfd pollfd;
#include "Client.hpp"
#include <map>
class Server
{
	private:
		int _lfd;
		int _port;
		std::string _password;
		std::vector<int> _client_fds;
		std::map<int, Client> _clients;
		Server();
	public:
		Server(int	port, const std::string &pass): _lfd(-1), _port(port), _password(pass){}
		std::vector<std::string> split_cmd(const std::string &cmd)
		{
			std::vector<std::string> tokens;
			std::istringstream stream(cmd);
			std::string token;
			while (stream >> token)
			{
				tokens.push_back(token);
				token.clear();
			}
			return tokens;
		}
		void	setAuthenticated(bool auth) { _authenticated = auth; };
		void	authenticate(std::string cmd, int fd);
		void	set_nickname(std::string cmd, int fd);
		void	set_username(std::vector<std::string> tokens, int fd);
		void	disconnect(std::string cmd, int fd);
		void	parse_cmd(const std::string &cmd, int fd)
		{
			std::vector<std::string> tokens = split_cmd(cmd);
			if (!tokens.empty() && (tokens[0] == "PASS" || tokens[0] == "pass"))
				authenticate(cmd, fd);
			else if (!tokens.empty() && (tokens[0] == "NICK" || tokens[0] == "nick"))
				set_nickname(cmd, fd);
			else if (!tokens.empty() && (tokens[0] == "USER" || tokens[0] == "user"))
				set_username(tokens, fd);
			else if (!tokens.empty() && (tokens[0] == "QUIT" || tokens[0] == "quit"))
				disconnect(cmd, fd);
			else if ()
			{
				if (tokens.empty() && (tokens[0] == "invite" || tokens[0] == "invite"))
					return;
			}
			else
			{
				
				send()
			}
		}
		void	setup()
		{
			sockaddr_in s;
			memset(&s, 0, sizeof(s));
			s.sin_family = AF_INET;
			s.sin_addr.s_addr = INADDR_ANY;
			s.sin_port = htons(_port);
			_lfd = socket(AF_INET, SOCK_STREAM, 0);
			if (_lfd < 0)
				throw std::runtime_error("Socket creation failed");
			int opt = 1;
			if (setsockopt(_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
				throw std::runtime_error("setsockopt failed");
			if (bind(_lfd, (struct sockaddr *)&s, sizeof(s)) < 0)
				throw std::runtime_error("Bind failed");
			if (listen(_lfd, 10) < 0)
				throw std::runtime_error("Listen failed");
			if (fcntl(_lfd, F_SETFL, O_NONBLOCK) < 0)
				throw std::runtime_error("fcntl failed");
		}
		/* struct pollfd 
               int   fd;        ///////file descriptor ///
               short events;    ///////requested events ///
               short revents;   ///////returned events ///
         */
		void Run()
		{
			
			std::vector<pollfd> fds;
			pollfd lfd;
			lfd.fd = _lfd;
			lfd.events = POLLIN;
			lfd.revents = 0;
			fds.push_back(lfd);
		
			while (true)
			{
				std::cout << "waiting clinet input" << std::endl;
				if (poll(fds.data(), fds.size(), -1) < 0)
				{
					if (errno == EINTR)
						continue;
					throw std::runtime_error("poll failed");
				}
				for (size_t i = 0; i < fds.size();i++)
				{
					if (fds[i].revents & POLLIN)
					{
						if (fds[i].fd == _lfd)
						{
							int incoming_call = accept(_lfd, NULL, NULL);
							if (incoming_call < 0)
								continue;
							int incoming_call_state = fcntl(incoming_call, F_SETFL,O_NONBLOCK); // non block kat3ni anaha lmain thread matb9ach tsna taidir lfd lakhor chi input b7al kima kna kandiro f minishell cat katsna input hadi la kadesactiviha 
							if (incoming_call_state < 0)												// F_SETF == File discriptor SET FL.AG 
							{
								close(incoming_call);
								continue; // ma3ndkch mo3idat azpi skipi talfo9 again;!!!!
							}
							pollfd lmo3idat; // hna kadir lmo3idat lljondi  jdid okat7to ftiara dial free fire 
							lmo3idat.fd = incoming_call;
							lmo3idat.events = POLLIN;
							lmo3idat.revents = 0;
							fds.push_back(lmo3idat);
							_clients.insert(std::make_pair(incoming_call, Client(incoming_call))); // hna katpushi lclient jdid fmap dial clients li kayn f server

						}
						else // hna katchecki mn b3d okatla9a bl jondi li deja pushitih fliteration lwla okat9ol lih yala tla7 ojib lia 
						{
								char buffer [1024];
								ssize_t size = recv(fds[i].fd,buffer,sizeof(buffer) - 1,0);
								if (size <= 0)
								{
									// 0 sdha mn raso or client deconecta saf 
									// < 0 rah kain issue f socket handlih t7wa
									int dead_fd = fds[i].fd; // khodo 9bel materasi, mn b3d lerase fds[i] rah wa7d akhor
									close(dead_fd);
									_clients.erase(dead_fd);
									fds.erase(fds.begin() + i);
									i--;
									continue;
								}
								buffer[size] = '\0';
								Client &client = _clients[fds[i].fd];
								client.appendToBuffer(buffer, size);
								client.print();
								std::vector<std::string> cmds = client.splitBuffer(); // hna kat9ra lcommand li jiti mn client okat7to fbuffer dialo
								for (size_t j = 0; j < cmds.size(); j++)
									this->parse_cmd(cmds[j], fds[i].fd); // hna katparse lcommand li jiti mn client okat9raha ffunction parse_cmd li kayn f server.cpp
								
						}
					}
				}
				
			}
		}
		~Server()
		{
			if (_lfd >= 0)
				close(_lfd);
		}
};
#endif