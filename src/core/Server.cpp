#include <iostream>
#include <cstring>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "Server.hpp"

bool	Server::_running = false;

// Constr and destr
Server::Server(int port, const std::string &password)
	: _port(port), _password(password), _serverFd(-1)
{
	initSocket();
}

Server::~Server()
{
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		close(it->first);
	if (_serverFd != -1)
		close(_serverFd);
	std::cout << "Server shut down." << std::endl;
}


// init and stop
void	Server::stop() { _running = false; }

void	Server::initSocket()
{
	_serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_serverFd == -1)
		throw std::runtime_error("socket() failed");

	int opt = 1;
	if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		throw std::runtime_error("setsockopt() failed");

	std::memset(&_addr, 0, sizeof(_addr));
	_addr.sin_family      = AF_INET;
	_addr.sin_addr.s_addr = INADDR_ANY;
	_addr.sin_port        = htons(_port);

	if (bind(_serverFd, (struct sockaddr *)&_addr, sizeof(_addr)) == -1)
		throw std::runtime_error("bind() failed");

	if (listen(_serverFd, SOMAXCONN) == -1)
		throw std::runtime_error("listen() failed");

	if (fcntl(_serverFd, F_SETFL, O_NONBLOCK) == -1)
		throw std::runtime_error("fcntl() failed");

	pollfd pfd;
	pfd.fd     = _serverFd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	_pollfds.push_back(pfd);

	std::cout << "Server listening on port " << _port << std::endl;
}


// Util funcs
void	Server::sendToClient(int fd, const std::string &msg)
{
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it != _clients.end())
		it->second.sendMsg(msg);
}

Client	*Server::getClientByNick(const std::string &nick)
{
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		if (it->second.getNick() == nick)
			return (&it->second);
	return (NULL);
}

bool	Server::nickInUse(const std::string &nick) const
{
	for (std::map<int, Client>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
		if (it->second.getNick() == nick)
			return (true);
	return (false);
}

Channel	*Server::getChannel(const std::string &name)
{
	for (size_t i = 0; i < _channels.size(); ++i)
		if (_channels[i].getName() == name)
			return (&_channels[i]);
	return (NULL);
}

Channel	&Server::getOrCreateChannel(const std::string &name)
{
	Channel *c = getChannel(name);
	if (c) return (*c);
	_channels.push_back(Channel(name));
	return (_channels.back());
}



//main poll loop
void	Server::run()
{
	_running = true;
	while (_running)
	{
		int n = poll(&_pollfds[0], _pollfds.size(), 500);
		if (n < 0)
		{
			if (_running)
				std::cerr << "poll() interrupted" << std::endl;
			break;
		}
		if (n == 0) continue;

		for (size_t i = 0; i < _pollfds.size(); ++i)
		{
			if (_pollfds[i].revents == 0) continue;
			int fd = _pollfds[i].fd;

			if (_pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				if (fd != _serverFd)
				{
					disconnectClient(fd);
					--i;
					continue;
				}
			}

			if (_pollfds[i].revents & POLLIN)
			{
				if (fd == _serverFd)
					acceptNewClient();
				else
				{
					handleClientData(fd);
					if (_clients.find(fd) == _clients.end())
						--i;
				}
			}
		}
	}
}

// Parse
void	Server::processMessage(int fd, const IrcMessage &msg)
{
	(void)fd;
	std::cout << "[recv] " << msg.toString() << std::endl;
	// entrega_1: solo loguea, todavia no hay comandos
}
