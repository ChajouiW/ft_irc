#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

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

	public:
		Client() : _fd(-1), _buffer() {}
		Client(int fd) : _fd(fd), _buffer() {}
		~Client() {}

		int	getFd() const { return _fd; }

		// Append the bytes just read from the socket. Length aware: recv() data
		// is not a C string, it can legitimately contain a '\0'.
		void	appendToBuffer(const char *data, size_t size)
		{
			_buffer.append(data, size);
		}

		// Pull one complete line (terminated by \r\n) out of the buffer.
		// Returns true and fills `line` when a full command is available,
		// false when the buffer only holds a partial one.
		// Body comes in Slice 4.
		std::string	extractCommand()
		{
			return _buffer;
		};
};

#endif // CLIENT_HPP
