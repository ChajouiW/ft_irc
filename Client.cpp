#include "Client.hpp"

#include <iostream>

Client::Client(int fd, const std::string &ip) : _fd(fd), _ip(ip), _buffer(""), _nick(""), _user(""), _realname(""), _pass(false), _registered(false) {}

Client::Client(const Client &other) : _fd(other._fd), _ip(other._ip), _buffer(other._buffer), _nick(other._nick), _user(other._user), _realname(other._realname), _pass(other._pass), _registered(other._registered) {}

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
	// std::cout << "Client fd: " << _fd << ", buffer: " << _buffer << std::endl;
}

std::string	Client::getBuffer() const { return _buffer; }

/*
** 3lach 7ayadna std::getline / istringstream mn hna:
**
** 1) getline kayrje3 SUCCESS 7ta ila ma l9ach '\n' — kaykmel 3la end-of-stream.
**    donc "com" (bla delimiter) o "com\n" kaytiw bjouj nafs lresultat: got["com"].
**    lma3lomat li 7tajinaha — WACH kan kayn delimiter — kadi3 3ndna, o partial
**    line katb9a tban b7al chi command kamla. hadi hiya l3ilaj dial ctrl+D
**    (com^Dman^Dd) f subject: kola flush kayb9a command bo7do.
**
** 2) `_buffer = _buffer.substr(pos + 2)` — pos huwa index f `line` machi f
**    `_buffer`, o +2 kaftared belli '\r' o '\n' bjouj wslo. ila wsel ghir '\r'
**    (packet t9ta3 binathom): _buffer = "A\r" (size 2), substr(3) =>
**    std::out_of_range => terminate => server mat => note 0.
**
** l7al: kanl9aw '\n' nichan f _buffer, kanhesbo koulchi b coordonnées dial
** _buffer, o kanmes7o pos+1 (ghir '\n' li l9ina b3inina).
*/
std::vector<std::string>	Client::splitBuffer()
{
	std::vector<std::string> cmds;
	size_t pos;

	// kanl9aw '\n' f _buffer nichan: ila makanch, ma kayn ta command kamla,
	// kanhaydo walo o kankhelliw lba9i f buffer 7ta tji chi packet khra.
	while ((pos = _buffer.find('\n')) != std::string::npos)
	{
		std::string line = _buffer.substr(0, pos);
		// '\r' optionnel: nc bla -C kaysift ghir '\n'
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		_buffer.erase(0, pos + 1); // +1 = '\n' li l9ina, machi +2 dial delimiter li ma kaynch
		cmds.push_back(line);
	}
	return cmds;
}
