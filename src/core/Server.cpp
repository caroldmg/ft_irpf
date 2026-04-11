#include <iostream>
#include <cstring>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "Server.hpp"

Server::Server(int port, const std::string &password)
	: _port(port), _password(password), _serverFd(-1)
{
	initSocket();
}

Server::~Server()
{
	// cierra todos los clientes
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		close(it->first);

	// cierra el socket del servidor
	if (_serverFd != -1)
		close(_serverFd);

	std::cout << "Server shut down." << std::endl;
}

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
	_pollfds.push_back(pfd);

	std::cout << "Server listening on port " << _port << std::endl;
}