#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <cstddef>

/*
** Phase 3: a Client is just "a socket + the bytes we have read from it so far".
** Nickname / username / registration state come in Phase 5.
**
** The Client does NOT own the fd: it never closes it. The Server opened the
** socket (accept) and the Server closes it. Keeping ownership on one side
** avoids double close when a Client is copied into the container.
*/
class Client
{
		private:
		int			_fd;
		std::string	_buffer;
		std::string	_nickname;
		std::string	_username;
		bool		_authenticated;
		bool		_registered;
		std::string _write_buffer;
		protected:

		public:
		Client();
		Client(int fd);
		~Client();

		void	setAuthenticated(bool auth);
		bool	isAuthenticated() const;
		void	setRegistered(bool reg);
		bool	isRegistered() const;
		int		getFd() const;
		void 	
		void	appendToBuffer(const char *data, size_t size);
		void	print() const;
		bool	extractCommand(std::string &line);

		/*
			//extract
			parser output
			PRIVMSG #sdiji :Hello, world!

			cmd : PRIVMSG
			params : #sdiji
			trailing : Hello, world!





		*/
		// std::string	getBuffer() const { return _buffer; };
		// std::vector<std::string>	splitBuffer()
		// {
		// 	std::vector<std::string> cmds;
		// 	std::istringstream stream(_buffer);
		// 	std::string line;
		// 	while (std::getline(stream, line))
		// 	{
		// 		size_t pos = line.find_first_of("\r\n");
		// 		if (pos != std::string::npos)
		// 			line = line.substr(0, pos);
		// 		cmds.push_back(line);
		// 	}
		// 	return cmds;
		// };
};

#endif // CLIENT_HPP
