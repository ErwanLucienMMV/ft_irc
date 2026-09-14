#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <set>
#include <string>

class Channel
{
	public:
		Channel(const std::string &name, int creatorFd);

		const std::string &getName() const;
		const std::string &getTopic() const;
		const std::string &getKey() const;
		std::size_t getLimit() const;
		const std::set<int> &getMembers() const;

		bool hasMember(int fd) const;
		bool isOperator(int fd) const;
		bool isInvited(int fd) const;
		bool isInviteOnly() const;
		bool isTopicRestricted() const;
		bool isFull() const;
		bool isEmpty() const;

		void addMember(int fd);
		void removeMember(int fd);
		void addOperator(int fd);
		void removeOperator(int fd);
		void invite(int fd);
		void removeInvite(int fd);
		void setTopic(const std::string &topic);
		void setKey(const std::string &key);
		void setLimit(std::size_t limit);
		void setInviteOnly(bool enabled);
		void setTopicRestricted(bool enabled);

	private:
		std::string _name;
		std::string _topic;
		std::string _key;
		std::set<int> _members;
		std::set<int> _operators;
		std::set<int> _invited;
		std::size_t _limit;
		bool _inviteOnly;
		bool _topicRestricted;
};

#endif
