#include "bot.hpp"

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <ctime>
#include <cctype>
#include <sstream>
#include <stdexcept>

#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

Bot::Bot(const std::string &host, int port,
		 const std::string &password, const std::string &nick)
	: _fd(-1), _host(host), _port(port), _password(password), _nick(nick)
{
	std::srand(static_cast<unsigned int>(std::time(NULL)));
}

Bot::~Bot()
{
	if (_fd != -1)
		close(_fd);
}

void	Bot::sendRaw(const std::string &msg) const
{
	if (_fd != -1)
		send(_fd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
}

// Resuelve host:port (acepta IP o hostname) y abre la conexión TCP.
void	Bot::connectToServer()
{
	struct addrinfo hints;
	struct addrinfo *res = NULL;

	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	std::ostringstream oss;
	oss << _port;

	if (getaddrinfo(_host.c_str(), oss.str().c_str(), &hints, &res) != 0 || !res)
		throw std::runtime_error("Bot: no se pudo resolver el host");

	_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (_fd == -1)
	{
		freeaddrinfo(res);
		throw std::runtime_error("Bot: socket() falló");
	}

	if (connect(_fd, res->ai_addr, res->ai_addrlen) == -1)
	{
		freeaddrinfo(res);
		close(_fd);
		_fd = -1;
		throw std::runtime_error("Bot: connect() falló");
	}

	freeaddrinfo(res);
	registerBot();
}

void	Bot::registerBot()
{
	sendRaw("PASS " + _password + "\r\n");
	sendRaw("NICK " + _nick + "\r\n");
	sendRaw("USER " + _nick + " 0 * :PingPong Bot\r\n");
	std::cout << "[bot] registrado como " << _nick << std::endl;
}

// Genera "8====D" con un número aleatorio de '=' y una medida coherente.
std::string	Bot::makePpSize()
{
	int n = (std::rand() % 20) + 1;          // 1..20 signos '='
	std::string shaft(n, '=');
	int cm = n + (std::rand() % 5);           // medida en cm, ~ proporcional

	std::ostringstream oss;
	oss << "8" << shaft << "D  (" << cm << " cm)";
	return (oss.str());
}

// Reacciona a un PRIVMSG. Responde si el texto contiene "ppsize".
// Si el mensaje vino a un canal (#...), responde al canal; si fue directo,
// responde al remitente.
void	Bot::onPrivmsg(const std::string &sender, const std::string &target,
					   const std::string &text)
{
	std::string lowered = text;
	for (size_t i = 0; i < lowered.size(); ++i)
		lowered[i] = static_cast<char>(std::tolower(lowered[i]));

	if (lowered.find("ppsize") == std::string::npos)
		return;

	std::string replyTo = (!target.empty() && target[0] == '#') ? target : sender;
	if (replyTo.empty())
		return;

	sendRaw("PRIVMSG " + replyTo + " :" + makePpSize() + "\r\n");
	std::cout << "[bot] ppsize -> " << replyTo << std::endl;
}

// Procesa una línea IRC completa (sin el \r\n).
void	Bot::handleLine(const std::string &line)
{
	if (line.empty())
		return;

	// PING <token>  →  PONG <token>
	if (line.compare(0, 5, "PING ") == 0)
	{
		sendRaw("PONG " + line.substr(5) + "\r\n");
		return;
	}

	// Formato esperado:  :nick!user@host COMANDO args... [:trailing]
	std::string prefix;
	std::string rest = line;
	if (line[0] == ':')
	{
		size_t sp = line.find(' ');
		if (sp == std::string::npos)
			return;
		prefix = line.substr(1, sp - 1);
		rest = line.substr(sp + 1);
	}

	// nick = parte del prefijo antes de '!'
	std::string sender = prefix;
	size_t bang = sender.find('!');
	if (bang != std::string::npos)
		sender = sender.substr(0, bang);

	// comando
	size_t sp2 = rest.find(' ');
	std::string command = (sp2 == std::string::npos) ? rest : rest.substr(0, sp2);
	std::string args = (sp2 == std::string::npos) ? "" : rest.substr(sp2 + 1);

	if (command == "PRIVMSG")
	{
		// args = <target> :<text>
		size_t sp3 = args.find(' ');
		if (sp3 == std::string::npos)
			return;
		std::string target = args.substr(0, sp3);
		std::string text = args.substr(sp3 + 1);
		if (!text.empty() && text[0] == ':')
			text = text.substr(1);
		onPrivmsg(sender, target, text);
	}
	else if (command == "INVITE")
	{
		// :inviter INVITE <bot> :#canal   ó   <bot> #canal
		std::string chan = args;
		size_t colon = chan.find(':');
		if (colon != std::string::npos)
			chan = chan.substr(colon + 1);
		else
		{
			size_t sp4 = chan.find(' ');
			if (sp4 != std::string::npos)
				chan = chan.substr(sp4 + 1);
		}
		// limpia espacios sobrantes
		while (!chan.empty() && (chan[0] == ' '))
			chan = chan.substr(1);
		if (!chan.empty() && chan[0] == '#')
		{
			sendRaw("JOIN " + chan + "\r\n");
			std::cout << "[bot] invitado, uniéndome a " << chan << std::endl;
		}
	}
}

void	Bot::run()
{
	char buf[1024];

	std::cout << "[bot] escuchando. Envíale 'ppsize' por PRIVMSG o en un canal."
			  << std::endl;
	while (true)
	{
		ssize_t n = recv(_fd, buf, sizeof(buf), 0);
		if (n <= 0)
		{
			std::cout << "[bot] conexión cerrada por el servidor." << std::endl;
			break;
		}
		_buffer.append(buf, n);

		size_t pos;
		while ((pos = _buffer.find('\n')) != std::string::npos)
		{
			std::string line = _buffer.substr(0, pos);
			_buffer.erase(0, pos + 1);
			if (!line.empty() && line[line.size() - 1] == '\r')
				line.erase(line.size() - 1);
			handleLine(line);
		}
	}
}
