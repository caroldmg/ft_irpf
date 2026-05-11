#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Server.hpp"

void	Server::acceptNewClient()
{
	struct sockaddr_in	clientAddr;
	socklen_t			clientLen = sizeof(clientAddr);

	int clientFd = accept(_serverFd, (struct sockaddr *)&clientAddr, &clientLen);
	if (clientFd == -1)
	{
		std::cerr << "accept() failed" << std::endl;
		return;
	}

	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1)
	{
		std::cerr << "fcntl() failed for client fd" << std::endl;
		close(clientFd);
		return;
	}

	Client client(clientFd);
	client.setHost(inet_ntoa(clientAddr.sin_addr));
	_clients[clientFd] = client;

	pollfd pfd;
	pfd.fd     = clientFd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	_pollfds.push_back(pfd);

	std::cout << "New client connected: fd=" << clientFd
	          << " host=" << client.getHost() << std::endl;
}

void	Server::disconnectClient(int fd)
{
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it != _clients.end())
	{
		Client *c = &it->second;
		for (std::vector<Channel>::iterator ch = _channels.begin(); ch != _channels.end(); ++ch)
		{
			if (ch->isMember(c))
			{
				ch->broadcastExcept(":" + c->getPrefix() + " QUIT :connection closed\r\n", fd);
				ch->removeMember(c);
			}
		}
	}

	close(fd);
	_clients.erase(fd);
	removeFromPollfds(fd);

	std::cout << "Client disconnected: fd=" << fd << std::endl;
}

void	Server::removeFromPollfds(int fd)
{
	for (std::vector<pollfd>::iterator it = _pollfds.begin(); it != _pollfds.end(); ++it)
	{
		if (it->fd == fd)
		{
			_pollfds.erase(it);
			return;
		}
	}
}
