#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <sstream>
#include <vector>
#include <iostream>
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
		std::string _ip;
		std::string	_buffer;
		std::string	_nick;
		std::string _user;
		std::string _realname;
		bool		_pass;
		bool		_registered;

	public:
		Client(int fd, const std::string &ip) : _fd(fd), _ip(ip), _buffer(""), _nick(""), _user(""), _realname(""), _pass(false), _registered(false) {}
		Client(const Client &other) : _fd(other._fd), _ip(other._ip), _buffer(other._buffer), _nick(other._nick), _user(other._user), _realname(other._realname), _pass(other._pass), _registered(other._registered) {}
		Client(){};
		~Client() {}
		
		
		void	setFd(int fd) { _fd = fd; }
		void	setPass(bool pass) { _pass = pass; }
		void	setRegistered(bool reg) { _registered = reg; }
		void	setNick(const std::string &nick) { _nick = nick; }
		void	setUser(const std::string &user) { _user = user; }
		void	setRealname(const std::string &realname) { _realname = realname; }


		bool	isPass() const { return _pass; }
		bool	isRegistered() const { return _registered; }
		std::string	getNick() const { return _nick; }
		std::string	getUser() const { return _user; }
		std::string	getRealname() const { return _realname; }
		std::string	getIp() const { return _ip; }
		int	getFd() const { return _fd; }

		// Append the bytes just read from the socket. Length aware: recv() data
		// is not a C string, it can legitimately contain a '\0'.
		void	appendToBuffer(const char *data, size_t size)
		{
			_buffer.append(data, size);
		}
		void	print() const
		{
			for (size_t i = 0; i < _buffer.size(); i++)
			{
				if (_buffer[i] == '\r')
					std::cout << "\\r";
				else if (_buffer[i] == '\n')
					std::cout << "\\n ";
				else
					std::cout << _buffer[i];
			}
			// std::cout << "Client fd: " << _fd << ", buffer: " << _buffer << std::endl;
		}
		std::string	getBuffer() const { return _buffer; };
		// Pull one complete line (terminated by \r\n) out of the buffer.
		// Returns true and fills `line` when a full command is available,
		// false when the buffer only holds a partial one.
		// Body comes in Slice 4.
		std::vector<std::string>	splitBuffer()
		{
			std::vector<std::string> cmds;
			std::istringstream stream(_buffer);
			std::string line;
			while (std::getline(stream, line))
			{
				size_t pos = line.find_first_of("\r\n");
				if (pos != std::string::npos)
				{
					line = line.substr(0, pos);
					_buffer = _buffer.substr(pos + 2);
					cmds.push_back(line);
				}
			}
			return cmds;
		};
};

#endif // CLIENT_HPP
