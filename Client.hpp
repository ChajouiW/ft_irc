#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>
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
		std::string _ip;
		std::string	_buffer;
		std::string	_nick;
		std::string _user;
		std::string _realname;
		bool		_pass;
		bool		_registered;

	public:
		Client(int fd, const std::string &ip);
		Client(const Client &other);
		Client();
		~Client();


		void	setFd(int fd);
		void	setPass(bool pass);
		void	setRegistered(bool reg);
		void	setNick(const std::string &nick);
		void	setUser(const std::string &user);
		void	setRealname(const std::string &realname);


		bool	isPass() const;
		bool	isRegistered() const;
		std::string	getNick() const;
		std::string	getUser() const;
		std::string	getRealname() const;
		std::string	getIp() const;
		int	getFd() const;

		// Append the bytes just read from the socket. Length aware: recv() data
		// is not a C string, it can legitimately contain a '\0'.
		void	appendToBuffer(const char *data, size_t size);
		void	print() const;
		std::string	getBuffer() const;
		// Pull one complete line (terminated by \r\n) out of the buffer.
		// Returns true and fills `line` when a full command is available,
		// false when the buffer only holds a partial one.
		// Body comes in Slice 4.
		std::vector<std::string>	splitBuffer();
};

#endif // CLIENT_HPP
