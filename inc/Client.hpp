#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>

class Client
{
	public:
		Client();
		Client(int fd);
		Client(const Client &other);
		Client &operator=(const Client &other);
		~Client();

		int					getFd()        const;
		const std::string	&getNick()     const;
		const std::string	&getUser()     const;
		const std::string	&getHost()     const;
		const std::string	&getRealname() const;
		bool				isRegistered() const;
		bool				hasPassOk()    const;
		bool				hasNick()      const;
		bool				hasUser()      const;

		void	setNick(const std::string &nick);
		void	setUser(const std::string &user);
		void	setHost(const std::string &host);
		void	setRealname(const std::string &realname);
		void	setPassOk(bool ok);

		void		appendToBuffer(const std::string &data);
		bool		hasCompleteLine() const;
		std::string	flushLine();

		void	sendMsg(const std::string &msg) const;

		std::string	getPrefix() const;

	private:
		int			_fd;
		std::string	_nick;
		std::string	_user;
		std::string	_host;
		std::string	_realname;
		std::string	_buffer;

		bool		_passOk;
		bool		_hasNick;
		bool		_hasUser;
};

#endif
