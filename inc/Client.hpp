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
 
		//  getters 
		int					getFd()        const;
		const std::string	&getNick()     const;
		const std::string	&getUser()     const;
		const std::string	&getHost()     const;
		const std::string	&getRealname() const;
		bool				isRegistered() const;
		bool				hasPassOk()    const;
		bool				hasNick()      const;
		bool				hasUser()      const;
 
		//  setters 
		void	setNick(const std::string &nick);
		void	setUser(const std::string &user);
		void	setHost(const std::string &host);
		void	setRealname(const std::string &realname);
		void	setPassOk(bool ok);
 
		//  buffer de recepcion 
		void		appendToBuffer(const std::string &data);
		std::string	getFirstLine();                             
		bool		hasCompleteLine() const;                  
 
		//  envio 
		void	sendMsg(const std::string &msg) const; //  msg con \r\n en el fd del cliente
 
		//  cabecera IRC 
		std::string	getPrefix() const; // formato nick!user@host 
 
	private:
		int		    fd;
		std::string nick;
		std::string user;
		std::string host;
		std::string realname;
		std::string buffer
 
		bool	    passOk;
		bool	    hasNick;
		bool	    hasUser;
};
 
#endif
 