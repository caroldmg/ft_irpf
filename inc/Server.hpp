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
 
class Client;
class Channel;
 
class Server
{
	public:
		Server(int port, const std::string &password);
		~Server();
 
		void	run();
 
	private:
		int					port;
		std::string			password;
		int					serverFd;
		struct sockaddr_in	addr;
 
		// estado
		std::map<int, Client>	clients;   // fd (socket) -> Client
		std::vector<Channel>	channels;
		std::vector<pollfd>		pollfds;
 
		// red
		void	initSocket();
		void	acceptNewClient();
		void	handleClientData(int fd);
		void	disconnectClient(int fd);
 
		// Parseo
		void	processMessage(int fd, const IrcMessage &msg);

        // Acciones comandos
		void	cmdPass(int fd, const IrcMessage &msg);
		void	cmdNick(int fd, const IrcMessage &msg);
		void	cmdUser(int fd, const IrcMessage &msg);
		void	cmdJoin(int fd, const IrcMessage &msg);
		void	cmdPart(int fd, const IrcMessage &msg);
		void	cmdPrivmsg(int fd, const IrcMessage &msg);
		void	cmdKick(int fd, const IrcMessage &msg);
		void	cmdInvite(int fd, const IrcMessage &msg);
		void	cmdTopic(int fd, const IrcMessage &msg);
		void	cmdMode(int fd, const IrcMessage &msg);
		void	cmdQuit(int fd, const IrcMessage &msg);
 
        // utils
		Client	*getClientByNick(const std::string &nick);
		Channel	*getChannel(const std::string &name);
		Channel	&getOrCreateChannel(const std::string &name);
		bool	nickInUse(const std::string &nick) const;
		void	sendToClient(int fd, const std::string &msg);
		void	removeFromPollfds(int fd);
 
		Server(const Server &);
		Server &operator=(const Server &);
};
 
#endif
 