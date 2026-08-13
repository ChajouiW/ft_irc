#include "Client.hpp"

#include <iostream>

Client::Client(int fd, const std::string &ip) : _fd(fd), _ip(ip), _buffer(""), _nick(""), _user(""), _realname(""), _pass(false), _registered(false) {}

Client::Client(const Client &other) : _fd(other._fd), _ip(other._ip), _buffer(other._buffer), _nick(other._nick), _user(other._user), _realname(other._realname), _pass(other._pass), _registered(other._registered), _writeBuffer(other._writeBuffer) {}

Client::Client(){}

Client::~Client() {}

void	Client::setFd(int fd) { _fd = fd; }

void	Client::setPass(bool pass) { _pass = pass; }

void	Client::setRegistered(bool reg) { _registered = reg; }

void	Client::setNick(const std::string &nick) { _nick = nick; }

void	Client::setUser(const std::string &user) { _user = user; }

void	Client::setRealname(const std::string &realname) { _realname = realname; }

bool	Client::isPass() const { return _pass; }

bool	Client::isRegistered() const { return _registered; }

std::string	Client::getNick() const { return _nick; }

std::string	Client::getUser() const { return _user; }

std::string	Client::getRealname() const { return _realname; }

std::string	Client::getIp() const { return _ip; }

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

std::string	Client::getBuffer() const { return _buffer; }

std::vector<std::string>	Client::splitBuffer()
{
	std::vector<std::string> cmds;
	size_t pos;

	while ((pos = _buffer.find('\n')) != std::string::npos)
	{
		std::string line = _buffer.substr(0, pos);
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		_buffer.erase(0, pos + 1);
		cmds.push_back(line);
	}
	return cmds;
}

void	Client::setWriteBuffer(const std::string &buffer) { _writeBuffer = buffer; }

std::string	Client::getWriteBuffer() const { return _writeBuffer; }

void	Client::appendWriteBuffer(const std::string &msg)
{
	_writeBuffer += msg;
}

bool	Client::hasPendingWrite() const
{
	return !_writeBuffer.empty();
}

void	Client::consumeWriteBuffer(size_t n)
{
	if (n >= _writeBuffer.size())
		_writeBuffer.clear();
	else
		_writeBuffer.erase(0, n);
}
