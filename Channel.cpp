#include "Channel.hpp"
#include "Server.hpp"

Channel::Channel() : _name(""), _key(""), _topic(""), _limit(0), _inviteOnly(false), _hasKey(false), _isTopicRestricted(false)
{}
Channel::Channel(const std::string &name, const std::string &key):_name(name), _key(key), _topic(""), _limit(0), _inviteOnly(false), _hasKey(!key.empty()), _isTopicRestricted(false)
{}

Channel::~Channel() {}

bool Channel::addMember(int fd)
{
	if (_limit > 0 && getChannelSize() >= _limit)
		return false;
	return _members.insert(fd).second;
}

bool Channel::addOperator(int fd)
{
	if (_limit > 0 && getChannelSize() >= _limit && !isInChannel(fd))
		return false;
	if (isMember(fd))
		_members.erase(fd);
	return _operators.insert(fd).second;
}

void	Channel::cancelInvits(int fd)
{
	if (_invited.find(fd) != _invited.end())
		_invited.erase(fd);
}

void	Channel::removeOperator(int fd)
{
	if (_operators.find(fd) != _operators.end())
	{
		_operators.erase(fd);
		_members.insert(fd);
	}
}

void	Channel::removeMember(int fd)
{
	if (_members.find(fd) != _members.end())
		_members.erase(fd);
	if (_operators.find(fd) != _operators.end())
		_operators.erase(fd);
}

bool	Channel::isMember(int fd) const
{
	return _members.find(fd) != _members.end();
}

bool	Channel::isOperator(int fd) const
{
	return _operators.find(fd) != _operators.end();
}

bool	Channel::isInChannel(int fd) const
{
	return isMember(fd) || isOperator(fd);
}

const std::set<int>& Channel::getOperators() const
{
	return _operators;
}

const std::set<int>& Channel::getMembers() const
{
	return _members;
}


const std::string& Channel::getName() const
{
	return _name;
}

const std::string& Channel::getKey() const
{
	return _key;
}

const std::string& Channel::getTopic() const
{
	return _topic;
}

int Channel::getLimit() const
{
	return _limit;
}

int	Channel::getChannelSize() const
{
	return _members.size() + _operators.size();
}

bool Channel::isInviteOnly() const
{
	return _inviteOnly;
}

bool Channel::hasKey() const
{
	return _hasKey;
}

bool Channel::isTopicRestricted() const
{
	return _isTopicRestricted;
}

bool Channel::isInvited(int fd) const
{
	return _invited.find(fd) != _invited.end();
}

void Channel::addInvited(int fd)
{
	_invited.insert(fd);
}

void Channel::removeInvited(int fd)
{
	_invited.erase(fd);
}

void Channel::setTopic(const std::string &topic)
{
	_topic = topic;
}

void Channel::setInviteOnly(bool inviteOnly)
{
	_inviteOnly = inviteOnly;
}

void Channel::setKey(const std::string &key)
{
	_key = key;
	_hasKey = !key.empty();
}

void Channel::setLimit(int limit)
{
	_limit = limit;
}

void Channel::setTopicRestricted(bool restricted)
{
	_isTopicRestricted = restricted;
}


