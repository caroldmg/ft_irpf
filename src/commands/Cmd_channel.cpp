#include "Server.hpp"
#include "Replies.hpp"

#include <cstdlib>
#include <sstream>

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

void	Server::cmdPrivmsg(int fd, const IrcMessage &msg)
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
		sendReply(fd, ERR_NORECIPIENT, ":No recipient given (PRIVMSG)");
		return;
	}
	if (msg.params.size() < 2 || msg.params[1].empty())
	{
		sendReply(fd, ERR_NOTEXTTOSEND, ":No text to send");
		return;
	}

	const std::string &target = msg.params[0];
	const std::string &text   = msg.params[1];

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

void	Server::cmdMode(int fd, const IrcMessage &msg)
{
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it == _clients.end()) return;
	Client &c = it->second;

	if (!c.isRegistered())
	{
		sendReply(fd, ERR_NOTREGISTERED, ":You have not registered");
		return;
	}

	if (msg.params.size() < 2)
	{
		sendReply(fd, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
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

	if (!chan->isOperator(&c))
	{
		sendReply(fd, ERR_CHANOPRIVSNEEDED, chanName + " :You're not channel operator");
		return;
	}

	std::string flags;
	std::vector<std::string> args;

	for (size_t i = 1; i < msg.params.size(); ++i)
	{
		const std::string &param = msg.params[i];
		if (!param.empty() && (param[0] == '+' || param[0] == '-'))
			flags += param;
		else
			args.push_back(param);
	}

	bool adding = true;
	size_t argIndex = 0;

	for (size_t i = 0; i < flags.size(); ++i)
	{
		char flag = flags[i];

		if (flag == '+') { adding = true; continue; }
		if (flag == '-') { adding = false; continue; }

		switch (flag)
		{
			// ----------------------------- MODE +i / -i (invite only)   -----------------------------
			case 'i':
			{
				chan->setInviteOnly(adding);
				chan->broadcast(":" + c.getPrefix() + " MODE " + chanName + " " + (adding ? "+i" : "-i") + "\r\n");
				break;
			}

			// ----------------------------- MODE +t / -t (topic op only) -----------------------------
			case 't':
			{
				chan->setTopicOpOnly(adding);
				chan->broadcast(":" + c.getPrefix() + " MODE " + chanName + " " + (adding ? "+t" : "-t") + "\r\n");
				break;
			}

			// ----------------------------- MODE +k / -k (key)           -----------------------------
			case 'k':
			{
				if (adding)
				{
					if (argIndex >= args.size())
					{
						sendReply(fd, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters for +k");
						return;
					}

					const std::string &key = args[argIndex++];
					chan->setKey(key);
					chan->broadcast(":" + c.getPrefix() + " MODE " + chanName + " +k " + key + "\r\n");
				}
				else
				{
					if (argIndex < args.size())
						argIndex++;
				
					chan->unsetKey();
					chan->broadcast(":" + c.getPrefix() + " MODE " + chanName + " -k\r\n");
				}
				break;
			}

			// ----------------------------- MODE +o nick / -o nick (operator)    -----------------------------
			case 'o':
			{
				if (argIndex >= args.size())
				{
					sendReply(fd, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters for +o/-o");
					return;
				}

				const std::string &nick = args[argIndex++];
				Client *target = getClientByNick(nick);
				
				if (!target || !chan->isMember(target))
				{
					sendReply(fd, ERR_USERNOTINCHANNEL, nick + " " + chanName + " :They aren't on that channel");
					break;
				}

				if (adding)
					chan->addOperator(target);
				else
					chan->removeOperator(target);

				chan->broadcast(":" + c.getPrefix() + " MODE " + chanName + " " + (adding ? "+o " : "-o ") + nick + "\r\n");
				break;
			}

			// ----------------------------- MODE +l / -l (user limit)    -----------------------------
			case 'l':
			{
				if (adding)
				{
					if (argIndex >= args.size())
					{
						sendReply(fd, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters for +l");
						return;
					}

					int limit = atoi(args[argIndex++].c_str());
					chan->setUserLimit(limit);

					std::ostringstream oss;
					oss << limit;
					std::string limitStr = oss.str();
					chan->broadcast(":" + c.getPrefix() + " MODE " + chanName + " +l " + limitStr + "\r\n");
				}
				else
				{
					chan->unsetUserLimit();
					chan->broadcast(":" + c.getPrefix() + " MODE " + chanName + " -l\r\n");
				}
				break;
			}

			default:
			{
				sendReply(fd, ERR_UNKNOWNMODE, std::string(1, flag) + " :is unknown mode");
				break;
			}
		}
	}
}
