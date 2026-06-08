#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <map>
# include <vector>
# include <poll.h>
# include <netinet/in.h>

# include "Client.hpp"
# include "Channel.hpp"
# include "IrcMessage.hpp"

# define MAX_CLIENTS 100
# define SERVER_NAME "ircserv"

class Server
{
	public:
		Server(int port, const std::string &password);
		~Server();

		void	run();

		static void	stop();

	private:
		int					_port;
		std::string			_password;
		int					_serverFd;
		struct sockaddr_in	_addr;

		std::map<int, Client>	_clients;
		std::vector<Channel>	_channels;
		std::vector<pollfd>		_pollfds;

		static bool	_running;

		// red
		void	initSocket();
		void	acceptNewClient();
		void	handleClientData(int fd);
		void	disconnectClient(int fd);
		void	removeFromPollfds(int fd);
		void	sendToClient(int fd, const std::string &msg);

		// dispatch
		void	processMessage(int fd, const IrcMessage &msg);

		// comandos de registro
		void	cmdPass(int fd, const IrcMessage &msg);
		void	cmdNick(int fd, const IrcMessage &msg);
		void	cmdUser(int fd, const IrcMessage &msg);
		void	cmdQuit(int fd, const IrcMessage &msg);
		void	cmdPing(int fd, const IrcMessage &msg);
		void	cmdCap(int fd, const IrcMessage &msg);

		// comandos de canal
		void	cmdJoin(int fd, const IrcMessage &msg);
		void	cmdPart(int fd, const IrcMessage &msg);
		void	cmdKick(int fd, const IrcMessage &msg);
		void	cmdPrivmsg(int fd, const IrcMessage &msg);
		void	cmdNotice(int fd, const IrcMessage &msg);
		void	cmdTopic(int fd, const IrcMessage &msg);
		void	cmdInvite(int fd, const IrcMessage &msg);

		// helpers
		void	tryWelcome(int fd);
		void	sendReply(int fd, const std::string &code, const std::string &body);
		bool	isValidNick(const std::string &nick) const;
				// carol
		void	cmdPrivMsgHelper(const std::string &target, const std::string &text, int fd, Client &c);

		// utils
		Client	*getClientByNick(const std::string &nick);
		Channel	*getChannel(const std::string &name);
		Channel	&getOrCreateChannel(const std::string &name);
		bool	nickInUse(const std::string &nick) const;

		Server(const Server &);
		Server &operator=(const Server &);
};

#endif
