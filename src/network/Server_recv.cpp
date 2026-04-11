#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include "Server.hpp"
#include "irc.hpp"

void	Server::run();

void	Server::handleClientData(int fd);

void	Server::processMessage(int fd, const IrcMessage &msg)
{
	if      (msg.command == "PASS")    cmdPass(fd, msg);
	else if (msg.command == "NICK")    cmdNick(fd, msg);
	else if (msg.command == "USER")    cmdUser(fd, msg);
	else if (msg.command == "JOIN")    cmdJoin(fd, msg);
	else if (msg.command == "PART")    cmdPart(fd, msg);
	else if (msg.command == "PRIVMSG") cmdPrivmsg(fd, msg);
	else if (msg.command == "KICK")    cmdKick(fd, msg);
	else if (msg.command == "INVITE")  cmdInvite(fd, msg);
	else if (msg.command == "TOPIC")   cmdTopic(fd, msg);
	else if (msg.command == "MODE")    cmdMode(fd, msg);
	else if (msg.command == "QUIT")    cmdQuit(fd, msg);
	// comandos desconocidos se ignoran
}