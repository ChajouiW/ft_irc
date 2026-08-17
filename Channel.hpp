#ifndef CHANNEL_HPP
#define CHANNEL_HPP
#include <set>
#include <string>

class Server;

class Channel
{
    private:
        std::string		_name;
        std::string		_key;
        std::string		_topic;
        std::set<int>	_members; // Store client file descriptors
        std::set<int>	_operators; // Store operator file descriptors
		std::set<int>	_invited; // Store invited client file descriptors
        int				_limit;
		bool			_inviteOnly;
		bool			_hasKey;
		bool			_isTopicRestricted;


    public:
		Channel();
		Channel(const std::string &name, const std::string &key);
        ~Channel();

        bool addMember(int fd);
		bool addOperator(int fd);
		void cancelInvits(int fd);
        void removeMember(int fd);
        bool isMember(int fd) const;
		bool isOperator(int fd) const;
		bool isInChannel(int fd) const;

		//getters
        const std::set<int>&	getMembers() const;
        const std::set<int>&	getOperators() const;
		const std::string&		getName() const;
		const std::string&		getKey() const;
        const std::string&		getTopic() const;
		

		int					getLimit() const;
		bool isInvited(int fd) const;
		bool isInviteOnly() const;
		bool hasKey() const;
		bool isTopicRestricted() const;

		void addInvited(int fd);
		void removeInvited(int fd);
        void setTopic(const std::string &topic);
		void setInviteOnly(bool inviteOnly);
		void setKey(const std::string &key);
		void setLimit(int limit);
		void setTopicRestricted(bool restricted);

};

#endif