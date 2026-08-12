#include "Client.hpp"

#include <iostream>

Client::Client() : _fd(-1), _buffer(), _authenticated(false), _registered(false) {}

Client::Client(int fd) : _fd(fd), _buffer(), _authenticated(false), _registered(false) {}

Client::~Client() {}

void	Client::setAuthenticated(bool auth) { _authenticated = auth; }

bool	Client::isAuthenticated() const { return _authenticated; }

void	Client::setRegistered(bool reg) { _registered = reg; }

bool	Client::isRegistered() const { return _registered; }

int	Client::getFd() const { return _fd; }

void	Client::appendToBuffer(const char *data, size_t size)
{
	_buffer.append(data, size);
}

void	Client::print() const
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
}

bool Client::extractCommand(std::string &line)
{
	size_t pos = _buffer.find('\n');
	if (pos == std::string::npos)
		return false;
	line = _buffer.substr(0, pos);
	if (!line.empty() && line[line.size() - 1] == '\r')
		line.erase(line.size() - 1);
	_buffer.erase(0, pos + 1);
	return true;
}
