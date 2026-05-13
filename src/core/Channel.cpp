#include <sstream>
#include "Channel.hpp"
#include "Client.hpp"

Channel::Channel()
	: _userLimit(-1), _inviteOnly(false), _topicOpOnly(false) {}

Channel::Channel(const std::string &name)
	: _name(name), _userLimit(-1), _inviteOnly(false), _topicOpOnly(false) {}

Channel::Channel(const Channel &o)
	: _name(o._name), _topic(o._topic), _key(o._key),
	  _userLimit(o._userLimit),
	  _inviteOnly(o._inviteOnly), _topicOpOnly(o._topicOpOnly),
	  _members(o._members), _operators(o._operators), _invited(o._invited) {}

Channel &Channel::operator=(const Channel &o)
{
	if (this != &o)
	{
		_name = o._name;
		_topic = o._topic;
		_key = o._key;
		_userLimit = o._userLimit;
		_inviteOnly = o._inviteOnly;
		_topicOpOnly = o._topicOpOnly;
		_members = o._members;
		_operators = o._operators;
		_invited = o._invited;
	}
	return (*this);
}

Channel::~Channel() {}

const std::string	&Channel::getName()     const { return (_name); }
const std::string	&Channel::getTopic()    const { return (_topic); }
const std::string	&Channel::getKey()      const { return (_key); }
int					Channel::getUserLimit() const { return (_userLimit); }

bool	Channel::isInviteOnly()  const { return (_inviteOnly); }
bool	Channel::isTopicOpOnly() const { return (_topicOpOnly); }
bool	Channel::hasKey()        const { return (!_key.empty()); }
bool	Channel::hasUserLimit()  const { return (_userLimit > 0); }

void	Channel::setTopic(const std::string &t) { _topic = t; }
void	Channel::setKey(const std::string &k)   { _key = k; }
void	Channel::unsetKey()                     { _key.clear(); }
void	Channel::setInviteOnly(bool v)          { _inviteOnly = v; }
void	Channel::setTopicOpOnly(bool v)         { _topicOpOnly = v; }
void	Channel::setUserLimit(int l)            { _userLimit = l; }
void	Channel::unsetUserLimit()               { _userLimit = -1; }

void	Channel::addMember(Client *c)
{
	if (!c) return;
	for (size_t i = 0; i < _members.size(); ++i)
		if (_members[i] == c) return;
	_members.push_back(c);
}

void	Channel::removeMember(Client *c)
{
	for (std::vector<Client *>::iterator it = _members.begin(); it != _members.end(); ++it)
		if (*it == c) { _members.erase(it); break; }
	removeOperator(c);
}

bool	Channel::isMember(Client *c) const
{
	for (size_t i = 0; i < _members.size(); ++i)
		if (_members[i] == c) return (true);
	return (false);
}

bool	Channel::isMember(const std::string &nick) const
{
	for (size_t i = 0; i < _members.size(); ++i)
		if (_members[i] && _members[i]->getNick() == nick) return (true);
	return (false);
}

int	Channel::memberCount() const { return (static_cast<int>(_members.size())); }

void	Channel::addOperator(Client *c)
{
	if (!c) return;
	for (size_t i = 0; i < _operators.size(); ++i)
		if (_operators[i] == c) return;
	_operators.push_back(c);
}

void	Channel::removeOperator(Client *c)
{
	for (std::vector<Client *>::iterator it = _operators.begin(); it != _operators.end(); ++it)
		if (*it == c) { _operators.erase(it); return; }
}

bool	Channel::isOperator(Client *c) const
{
	for (size_t i = 0; i < _operators.size(); ++i)
		if (_operators[i] == c) return (true);
	return (false);
}

void	Channel::addInvited(const std::string &nick)
{
	for (size_t i = 0; i < _invited.size(); ++i)
		if (_invited[i] == nick) return;
	_invited.push_back(nick);
}

bool	Channel::isInvited(const std::string &nick) const
{
	for (size_t i = 0; i < _invited.size(); ++i)
		if (_invited[i] == nick) return (true);
	return (false);
}

void	Channel::removeInvited(const std::string &nick)
{
	for (std::vector<std::string>::iterator it = _invited.begin(); it != _invited.end(); ++it)
		if (*it == nick) { _invited.erase(it); return; }
}

void	Channel::broadcast(const std::string &msg) const
{
	for (size_t i = 0; i < _members.size(); ++i)
		if (_members[i]) _members[i]->sendMsg(msg);
}

void	Channel::broadcastExcept(const std::string &msg, int exceptFd) const
{
	for (size_t i = 0; i < _members.size(); ++i)
		if (_members[i] && _members[i]->getFd() != exceptFd)
			_members[i]->sendMsg(msg);
}

std::string	Channel::getMemberList() const
{
	std::string out;
	for (size_t i = 0; i < _members.size(); ++i)
	{
		if (!_members[i]) continue;
		if (!out.empty()) out += " ";
		if (isOperator(_members[i])) out += "@";
		out += _members[i]->getNick();
	}
	return (out);
}

std::string	Channel::getModeString() const
{
	std::string m = "+";
	std::string args;
	if (_inviteOnly)   m += "i";
	if (_topicOpOnly)  m += "t";
	if (hasKey())      { m += "k"; args += " " + _key; }
	if (hasUserLimit())
	{
		std::ostringstream oss; oss << _userLimit;
		m += "l"; args += " " + oss.str();
	}
	return (m + args);
}

const std::vector<Client *>	&Channel::getMembers() const { return (_members); }
