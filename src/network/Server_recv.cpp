#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include "Server.hpp"
#include "IrcMessage.hpp"

void	Server::handleClientData(int fd)
{
	char buf[1024];
	ssize_t n = recv(fd, buf, sizeof(buf), 0);
	if (n <= 0)
	{
		disconnectClient(fd);
		return;
	}

	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it == _clients.end()) return;

	it->second.appendToBuffer(std::string(buf, n));

	while (it->second.hasCompleteLine())
	{
		std::string line = it->second.flushLine();
		if (line.empty()) continue;

		IrcMessage msg;
		if (!IrcMessage::parse(line, msg))
			continue;
		processMessage(fd, msg);

		// si el cliente fue desconectado durante el dispatch, salir
		if (_clients.find(fd) == _clients.end())
			return;
	}
}
