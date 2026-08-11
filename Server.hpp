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
									close(fds[i].fd);
									fds.erase(fds.begin() + i);
									_clients.erase(fds[i].fd);
									i--;
									continue;
								}
								buffer[size] = '\0';
								Client &client = _clients[fds[i].fd];
								client.appendToBuffer(buffer, size);
								std::string line;
								while (client.extractCommand(line))
								{
									std::cout << "Received command from client " << fds[i].fd << ": " << line << std::endl;
									// Here you would handle the command, e.g., parse it and respond accordingly.
								}						
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