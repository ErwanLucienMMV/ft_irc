#include "channel.hpp"

Channel::Channel(const std::string &name, int creatorFd)
	: _name(name), _limit(0), _inviteOnly(false), _topicRestricted(false)
{
	_members.insert(creatorFd);
	_operators.insert(creatorFd);
}

const std::string &Channel::getName() const { return _name; }
const std::string &Channel::getTopic() const { return _topic; }
const std::string &Channel::getKey() const { return _key; }
std::size_t Channel::getLimit() const { return _limit; }
const std::set<int> &Channel::getMembers() const { return _members; }

bool Channel::hasMember(int fd) const { return _members.count(fd) != 0; }
bool Channel::isOperator(int fd) const { return _operators.count(fd) != 0; }
bool Channel::isInvited(int fd) const { return _invited.count(fd) != 0; }
bool Channel::isInviteOnly() const { return _inviteOnly; }
bool Channel::isTopicRestricted() const { return _topicRestricted; }
bool Channel::isFull() const { return _limit != 0 && _members.size() >= _limit; }
bool Channel::isEmpty() const { return _members.empty(); }

void Channel::addMember(int fd) { _members.insert(fd); }

void Channel::removeMember(int fd)
{
	_members.erase(fd);
	_operators.erase(fd);
	_invited.erase(fd);
}

void Channel::addOperator(int fd) { _operators.insert(fd); }
void Channel::removeOperator(int fd) { _operators.erase(fd); }
void Channel::invite(int fd) { _invited.insert(fd); }
void Channel::removeInvite(int fd) { _invited.erase(fd); }
void Channel::setTopic(const std::string &topic) { _topic = topic; }
void Channel::setKey(const std::string &key) { _key = key; }
void Channel::setLimit(std::size_t limit) { _limit = limit; }
void Channel::setInviteOnly(bool enabled) { _inviteOnly = enabled; }
void Channel::setTopicRestricted(bool enabled) { _topicRestricted = enabled; }
