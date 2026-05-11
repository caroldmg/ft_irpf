#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include <string>
# include <vector>

class Client;

class Channel
{
	public:
		Channel();
		Channel(const std::string &name);
		Channel(const Channel &other);
		Channel &operator=(const Channel &other);
		~Channel();

		const std::string	&getName()     const;
		const std::string	&getTopic()    const;
		const std::string	&getKey()      const;
		int					getUserLimit() const;

		bool	isInviteOnly()  const;
		bool	isTopicOpOnly() const;
		bool	hasKey()        const;
		bool	hasUserLimit()  const;

		void	setTopic(const std::string &topic);
		void	setKey(const std::string &key);
		void	unsetKey();
		void	setInviteOnly(bool val);
		void	setTopicOpOnly(bool val);
		void	setUserLimit(int limit);
		void	unsetUserLimit();

		void	addMember(Client *client);
		void	removeMember(Client *client);
		bool	isMember(Client *client) const;
		bool	isMember(const std::string &nick) const;
		int		memberCount() const;

		void	addOperator(Client *client);
		void	removeOperator(Client *client);
		bool	isOperator(Client *client) const;

		void	addInvited(const std::string &nick);
		bool	isInvited(const std::string &nick) const;
		void	removeInvited(const std::string &nick);

		void	broadcast(const std::string &msg) const;
		void	broadcastExcept(const std::string &msg, int exceptFd) const;

		std::string	getMemberList() const;
		std::string	getModeString() const;

		const std::vector<Client *>	&getMembers() const;

	private:
		std::string	_name;
		std::string	_topic;
		std::string	_key;
		int			_userLimit;

		bool		_inviteOnly;
		bool		_topicOpOnly;

		std::vector<Client *>		_members;
		std::vector<Client *>		_operators;
		std::vector<std::string>	_invited;
};

#endif
