#include <unistd.h>
#include <sstream>
#include "Server.hpp"
#include "Replies.hpp"

void	Server::sendReply(int fd, const std::string &code, const std::string &body)
{
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it == _clients.end()) return;
	std::string msg = Reply::make(SERVER_NAME, code, it->second.getNick(), body);
	it->second.sendMsg(msg);
}

bool	Server::isValidNick(const std::string &nick) const
{
	if (nick.empty() || nick.size() > 30) return (false);
	char c = nick[0];
	if ((c >= '0' && c <= '9') || c == '#' || c == ':' || c == '$' || c == ' ')
		return (false);
	for (size_t i = 0; i < nick.size(); ++i)
	{
		char ch = nick[i];
		if (ch == ' ' || ch == ',' || ch == '*' || ch == '?' || ch == '!' || ch == '@'
			|| ch == '.' || ch == ':' || ch == '\r' || ch == '\n')
			return (false);
	}
	return (true);
}

void	Server::tryWelcome(int fd)
{
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it == _clients.end()) return;
	Client &c = it->second;
	if (!c.isRegistered()) return;

	sendReply(fd, RPL_WELCOME,
		":Welcome to the IRC network " + c.getPrefix());
	sendReply(fd, RPL_YOURHOST,
		":Your host is " SERVER_NAME ", running version 1.0");
	sendReply(fd, RPL_CREATED,
		":This server was created today");
	sendReply(fd, RPL_MYINFO,
		std::string(SERVER_NAME) + " 1.0 o itkol");
}

void	Server::cmdPass(int fd, const IrcMessage &msg)
{
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it == _clients.end()) return;
	Client &c = it->second;

	if (c.isRegistered())
	{
		sendReply(fd, ERR_ALREADYREGISTRED, ":You may not reregister");
		return;
	}
	if (msg.params.empty())
	{
		sendReply(fd, ERR_NEEDMOREPARAMS, "PASS :Not enough parameters");
		return;
	}
	if (msg.params[0] != _password)
	{
		sendReply(fd, ERR_PASSWDMISMATCH, ":Password incorrect");
		return;
	}
	c.setPassOk(true);
}

void	Server::cmdNick(int fd, const IrcMessage &msg)
{
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it == _clients.end()) return;
	Client &c = it->second;

	if (!c.hasPassOk())
	{
		sendReply(fd, ERR_PASSWDMISMATCH, ":Password required");
		return;
	}
	if (msg.params.empty())
	{
		sendReply(fd, ERR_NONICKNAMEGIVEN, ":No nickname given");
		return;
	}
	const std::string &newNick = msg.params[0];
	if (!isValidNick(newNick))
	{
		sendReply(fd, ERR_ERRONEUSNICKNAME, newNick + " :Erroneous nickname");
		return;
	}
	if (nickInUse(newNick) && c.getNick() != newNick)
	{
		sendReply(fd, ERR_NICKNAMEINUSE, newNick + " :Nickname is already in use");
		return;
	}

	std::string oldPrefix = c.getPrefix();
	bool wasRegistered = c.isRegistered();
	c.setNick(newNick);

	if (wasRegistered)
		c.sendMsg(":" + oldPrefix + " NICK :" + newNick + "\r\n");
	else
		tryWelcome(fd);
}

void	Server::cmdUser(int fd, const IrcMessage &msg)
{
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it == _clients.end()) return;
	Client &c = it->second;

	if (c.isRegistered())
	{
		sendReply(fd, ERR_ALREADYREGISTRED, ":You may not reregister");
		return;
	}
	if (!c.hasPassOk())
	{
		sendReply(fd, ERR_PASSWDMISMATCH, ":Password required");
		return;
	}
	if (msg.params.size() < 4)
	{
		sendReply(fd, ERR_NEEDMOREPARAMS, "USER :Not enough parameters");
		return;
	}
	c.setUser(msg.params[0]);
	c.setRealname(msg.params[3]);
	tryWelcome(fd);
}

void	Server::cmdQuit(int fd, const IrcMessage &msg)
{
	std::string reason = msg.params.empty() ? "Client quit" : msg.params[0];
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it != _clients.end())
	{
		std::string prefix = it->second.getPrefix();
		for (size_t i = 0; i < _channels.size(); ++i)
			if (_channels[i].isMember(&it->second))
				_channels[i].broadcastExcept(":" + prefix + " QUIT :" + reason + "\r\n", fd);
	}
	disconnectClient(fd);
}

void	Server::cmdPing(int fd, const IrcMessage &msg)
{
	std::string token = msg.params.empty() ? "" : msg.params[0];
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it == _clients.end()) return;
	it->second.sendMsg(":" SERVER_NAME " PONG " SERVER_NAME " :" + token + "\r\n");
}

void	Server::cmdCap(int fd, const IrcMessage &msg)
{
	// Handshake CAP minimo: respondemos CAP * LS :  para permitir clientes que negocian
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it == _clients.end()) return;
	if (msg.params.empty()) return;
	if (msg.params[0] == "LS" || msg.params[0] == "LIST")
		it->second.sendMsg(":" SERVER_NAME " CAP * LS :\r\n");
	else if (msg.params[0] == "REQ")
		it->second.sendMsg(":" SERVER_NAME " CAP * NAK :" + (msg.params.size() > 1 ? msg.params[1] : std::string("")) + "\r\n");
	else if (msg.params[0] == "END")
	{
		// fin de la negociacion CAP, sin accion
	}
}
