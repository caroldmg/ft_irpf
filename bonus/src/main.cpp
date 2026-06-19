#include "bot.hpp"

#include <iostream>
#include <cstdlib>
#include <string>

int	main(int argc, char **argv)
{
	if (argc < 4 || argc > 5)
	{
		std::cerr << "Uso: " << argv[0]
				  << " <host> <port> <password> [nick]" << std::endl;
		return (1);
	}

	std::string host = argv[1];
	int port = std::atoi(argv[2]);
	std::string password = argv[3];
	std::string nick = (argc == 5) ? argv[4] : "ppbot";

	if (port <= 0 || port > 65535)
	{
		std::cerr << "Puerto inválido: " << argv[2] << std::endl;
		return (1);
	}

	try
	{
		Bot bot(host, port, password, nick);
		bot.connectToServer();
		bot.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return (1);
	}
	return (0);
}
