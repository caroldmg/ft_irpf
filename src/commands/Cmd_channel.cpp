#include "Server.hpp"
#include "Replies.hpp"

static bool isValidChannelName(const std::string &name)
{
	if (name.size() < 2 || name[0] != '#') return (false);
	for (size_t i = 1; i < name.size(); ++i)
	{
		char c = name[i];
		if (c == ' ' || c == ',' || c == '\7' || c == '\r' || c == '\n')
			return (false);
	}
	return (true);
}

void	Server::cmdJoin(int fd, const IrcMessage &msg)
{
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it == _clients.end()) return;
	Client &c = it->second;

	if (!c.isRegistered())
	{
		sendReply(fd, ERR_NOTREGISTERED, ":You have not registered");
		return;
	}
	if (msg.params.empty())
	{
		sendReply(fd, ERR_NEEDMOREPARAMS, "JOIN :Not enough parameters");
		return;
	}

	const std::string &chanName = msg.params[0];
	const std::string  key      = (msg.params.size() > 1) ? msg.params[1] : "";

	if (!isValidChannelName(chanName))
	{
		sendReply(fd, ERR_NOSUCHCHANNEL, chanName + " :No such channel");
		return;
	}

	Channel &chan = getOrCreateChannel(chanName);

	if (chan.isMember(&c))
		return;

	if (chan.isInviteOnly() && !chan.isInvited(c.getNick()))
	{
		sendReply(fd, ERR_INVITEONLYCHAN, chanName + " :Cannot join channel (+i)");
		return;
	}
	if (chan.hasKey() && key != chan.getKey())
	{
		sendReply(fd, ERR_BADCHANNELKEY, chanName + " :Cannot join channel (+k)");
		return;
	}
	if (chan.hasUserLimit() && chan.memberCount() >= chan.getUserLimit())
	{
		sendReply(fd, ERR_CHANNELISFULL, chanName + " :Cannot join channel (+l)");
		return;
	}

	bool firstMember = (chan.memberCount() == 0);
	chan.addMember(&c);
	if (firstMember)
		chan.addOperator(&c); // si creas el canal eres admin
	chan.removeInvited(c.getNick());

	chan.broadcast(":" + c.getPrefix() + " JOIN :" + chanName + "\r\n");

	if (!chan.getTopic().empty())
		sendReply(fd, RPL_TOPIC, chanName + " :" + chan.getTopic());
	else
		sendReply(fd, RPL_NOTOPIC, chanName + " :No topic is set");

	sendReply(fd, RPL_NAMREPLY, "= " + chanName + " :" + chan.getMemberList());
	sendReply(fd, RPL_ENDOFNAMES, chanName + " :End of /NAMES list");
}

void	Server::cmdPart(int fd, const IrcMessage &msg)
{
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it == _clients.end()) return;
	Client &c = it->second;

	if (!c.isRegistered())
	{
		sendReply(fd, ERR_NOTREGISTERED, ":You have not registered");
		return;
	}
	if (msg.params.empty())
	{
		sendReply(fd, ERR_NEEDMOREPARAMS, "PART :Not enough parameters");
		return;
	}

	const std::string &chanName = msg.params[0];
	const std::string  reason   = (msg.params.size() > 1) ? msg.params[1] : c.getNick();

	Channel *chan = getChannel(chanName);
	if (!chan)
	{
		sendReply(fd, ERR_NOSUCHCHANNEL, chanName + " :No such channel");
		return;
	}
	if (!chan->isMember(&c))
	{
		sendReply(fd, ERR_NOTONCHANNEL, chanName + " :You're not on that channel");
		return;
	}

	chan->broadcast(":" + c.getPrefix() + " PART " + chanName + " :" + reason + "\r\n");
	chan->removeMember(&c);

	if (chan->memberCount() == 0)
	{
		for (std::vector<Channel>::iterator ci = _channels.begin(); ci != _channels.end(); ++ci)
		{
			if (ci->getName() == chanName)
			{
				_channels.erase(ci);
				break;
			}
		}
	}
}

void Server::cmdKick(int fd, const IrcMessage &msg)
{
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it == _clients.end()) return;
	Client &c = it->second;

	if (!c.isRegistered())
	{
		sendReply(fd, ERR_NOTREGISTERED, ":You have not registered");
		return;
	}
	if (msg.params.empty() || msg.params.size() < 3)
	{
		sendReply(fd, ERR_NEEDMOREPARAMS, "KICK :Not enough parameters");
		return;
	}

	// primero voy a poner que elimine a solo uno de cada vez, no sé si debería hacer que eliminase a varios?
	const std::string &chanName = msg.params[0];
	// const std::string  reason   = (msg.params.size() > 1) ? msg.params[1] : c.getNick();
	const std::string &target = msg.params[1];
	// para poder echar a un usuario necesito su cliente,
	const std::string &userToKick = msg.params[2];

	Channel *chan = getChannel(chanName);
	if (!chan)
	{
		sendReply(fd, ERR_NOSUCHCHANNEL, chanName + " :No such channel");
		return;
	}
	if (!chan->isMember(&c))
	{
		sendReply(fd, ERR_NOTONCHANNEL, chanName + " :You're not on that channel");
		return;
	}

	// añadir verificacion de si el usuario que está haciendo la accion es operador
	if (!chan->isOperator(&c))
	{
		sendReply(fd, ERR_NOTONCHANNEL, chanName + " :You're not a channel operator");
		return;
	}
	if (!chan->isMember(userToKick))
	{
		sendReply(fd, ERR_NOTONCHANNEL, chanName + " : User" + userToKick + " not on that channel");
	}
	
	Client *u = chan->getMemberByNick(userToKick);

	if (u != NULL)
	{
		chan->broadcast(":" + c.getPrefix() + " KICK " + userToKick + " from " + chanName + "\r\n"); //+ " :" + reason + "\r\n");
		chan->removeMember(u);
	}

}

void	Server::cmdPrivmsg(int fd, const IrcMessage &msg)
{
	// el fd es el cliente al que manda un privmsg
	// para mandarlo a varios será un bucle y ya?
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it == _clients.end()) return;
	Client &c = it->second;

	if (!c.isRegistered())
	{
		sendReply(fd, ERR_NOTREGISTERED, ":You have not registered");
		return;
	}
	if (msg.params.empty())
	{
		sendReply(fd, ERR_NORECIPIENT, ":No recipient given (PRIVMSG)");
		return;
	}
	if (msg.params.size() < 2 || msg.params[1].empty())
	{
		sendReply(fd, ERR_NOTEXTTOSEND, ":No text to send");
		return;
	}

	const std::string &text = msg.params.back();

	// aqui ahora voy a hacer la separacion de argumentos para localizar los targets y hacer el bucle ese
	for (std::size_t i = 0; i + 1 < msg.params.size(); ++i)
	{
		const std::string &target = msg.params[i];
		cmdPrivMsgHelper(target, text, fd, c);
	}
}

void	Server::cmdPrivMsgHelper(const std::string &target, const std::string &text, int fd, Client &c)
{

	if (!target.empty() && target[0] == '#')
	{
		Channel *chan = getChannel(target);
		if (!chan)
		{
			sendReply(fd, ERR_NOSUCHCHANNEL, target + " :No such channel");
			return;
		}
		if (!chan->isMember(&c))
		{
			sendReply(fd, ERR_CANNOTSENDTOCHAN, target + " :Cannot send to channel");
			return;
		}
		chan->broadcastExcept(":" + c.getPrefix() + " PRIVMSG " + target + " :" + text + "\r\n", fd);
	}
	else
	{
		Client *dest = getClientByNick(target);
		if (!dest)
		{
			sendReply(fd, ERR_NOSUCHNICK, target + " :No such nick/channel");
			return;
		}
		dest->sendMsg(":" + c.getPrefix() + " PRIVMSG " + target + " :" + text + "\r\n");
	}
}

void	Server::cmdNotice(int fd, const IrcMessage &msg)
{
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it == _clients.end()) return;
	Client &c = it->second;

	if (!c.isRegistered())
		return;
	if (msg.params.size() < 2 || msg.params[0].empty() || msg.params[1].empty())
		return;

	const std::string &target = msg.params[0];
	const std::string &text   = msg.params[1];

	if (!target.empty() && target[0] == '#')
	{
		Channel *chan = getChannel(target);
		if (!chan || !chan->isMember(&c))
			return;
		chan->broadcastExcept(":" + c.getPrefix() + " NOTICE " + target + " :" + text + "\r\n", fd);
	}
	else
	{
		Client *dest = getClientByNick(target);
		if (!dest)
			return;
		dest->sendMsg(":" + c.getPrefix() + " NOTICE " + target + " :" + text + "\r\n");
	}
}

void	Server::cmdTopic(int fd, const IrcMessage &msg)
{
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it == _clients.end()) return;
	Client &c = it->second;

	if (!c.isRegistered())
	{
		sendReply(fd, ERR_NOTREGISTERED, ":You have not registered");
		return;
	}
	if (msg.params.empty())
	{
		sendReply(fd, ERR_NEEDMOREPARAMS, "TOPIC :Not enough parameters");
		return;
	}

	const std::string &chanName = msg.params[0];
	Channel *chan = getChannel(chanName);
	if (!chan)
	{
		sendReply(fd, ERR_NOSUCHCHANNEL, chanName + " :No such channel");
		return;
	}
	if (!chan->isMember(&c))
	{
		sendReply(fd, ERR_NOTONCHANNEL, chanName + " :You're not on that channel");
		return;
	}

	// solo consulta
	if (msg.params.size() < 2)
	{
		if (chan->getTopic().empty())
			sendReply(fd, RPL_NOTOPIC, chanName + " :No topic is set");
		else
			sendReply(fd, RPL_TOPIC, chanName + " :" + chan->getTopic());
		return;
	}

	if (chan->isTopicOpOnly() && !chan->isOperator(&c))
	{
		sendReply(fd, ERR_CHANOPRIVSNEEDED, chanName + " :You're not channel operator");
		return;
	}

	const std::string &newTopic = msg.params[1];
	chan->setTopic(newTopic);
	chan->broadcast(":" + c.getPrefix() + " TOPIC " + chanName + " :" + newTopic + "\r\n");
}

