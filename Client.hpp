#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>
#include <cstddef>
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
		std::string _writeBuffer;

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
		void	setWriteBuffer(const std::string &buffer);

		std::string	getWriteBuffer() const;
		std::string getPrefix() const;
		void	appendWriteBuffer(const std::string &msg);
		bool	hasPendingWrite() const;
		void	consumeWriteBuffer(size_t n);

		bool	isPass() const;
		bool	isRegistered() const;
		std::string	getNick() const;
		std::string	getUser() const;
		std::string	getRealname() const;
		std::string	getIp() const;
		int	getFd() const;

		void	appendToBuffer(const char *data, size_t size);
		void	print() const;
		std::string	getBuffer() const;
		std::vector<std::string>	splitBuffer();
};

#endif
