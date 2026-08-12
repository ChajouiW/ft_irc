#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <sstream>
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
		bool		_authenticated;

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
					line = line.substr(0, pos);
				cmds.push_back(line);
			}
			return cmds;
		};
};

#endif // CLIENT_HPP
