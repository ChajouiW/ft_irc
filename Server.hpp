#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>

#include "Client.hpp"
#include "parser.hpp"

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
		Server(int	port, const std::string &pass);
		~Server();

		// std::vector<std::string> split_cmd(const std::string &cmd)
		// {
		// 	std::vector<std::string> tokens;
		// 	std::istringstream stream(cmd);
		// 	std::string token;
		// 	while (stream >> token)
		// 	{
		// 		tokens.push_back(token);
		// 		token.clear();
		// 	}
		// 	return tokens;
		// }
		// void	setAuthenticated(bool auth) { _authenticated = auth; };
		// void	authenticate(std::string cmd, int fd);
		// void	set_nickname(std::string cmd, int fd);
		// void	set_username(std::vector<std::string> tokens, int fd);
		// void	disconnect(std::string cmd, int fd);
		// void	parse_cmd(const std::string &cmd, int fd)
		// {
		// 	std::vector<std::string> tokens = split_cmd(cmd);
		// 	if (!tokens.empty() && (tokens[0] == "PASS" || tokens[0] == "pass"))
		// 		authenticate(cmd, fd);
		// 	else if (!tokens.empty() && (tokens[0] == "NICK" || tokens[0] == "nick"))
		// 		set_nickname(cmd, fd);
		// 	else if (!tokens.empty() && (tokens[0] == "USER" || tokens[0] == "user"))
		// 		set_username(tokens, fd);
		// 	else if (!tokens.empty() && (tokens[0] == "QUIT" || tokens[0] == "quit"))
		// 		disconnect(cmd, fd);
		// }

		std::string	toUpper(const std::string &s);
		void		parse_args(t_command &cmd, const std::string &rest);
		void		parse_command(const std::string &line, t_command &cmd, std::string &rest);
		void		printf_command(const t_command &cmd);
		void		dispatcher(const t_command &cmd, int fd);
		void		setup();
		void		Run();
};

#endif
