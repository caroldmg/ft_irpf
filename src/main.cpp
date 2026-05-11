#include <iostream>
#include <cstdlib>
#include <csignal>
#include <stdexcept>
#include <string>
#include "Server.hpp"

static void	handleSignal(int)
{
	Server::stop();
}

static bool	parsePort(const std::string &s, int &out)
{
	if (s.empty())
		return (false);
	for (size_t i = 0; i < s.size(); ++i)
		if (s[i] < '0' || s[i] > '9')
			return (false);
	long v = std::atol(s.c_str());
	if (v <= 0 || v > 65535)
		return (false);
	out = static_cast<int>(v);
	return (true);
}

int	main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
		return (1);
	}

	int port;
	if (!parsePort(argv[1], port))
	{
		std::cerr << "Error: invalid port (1-65535)" << std::endl;
		return (1);
	}

	std::string password = argv[2];
	if (password.empty())
	{
		std::cerr << "Error: password cannot be empty" << std::endl;
		return (1);
	}

	std::signal(SIGINT, handleSignal);
	std::signal(SIGTERM, handleSignal);
	std::signal(SIGPIPE, SIG_IGN);

	try
	{
		Server server(port, password);
		server.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Fatal: " << e.what() << std::endl;
		return (1);
	}
	return (0);
}
