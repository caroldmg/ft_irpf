#ifndef BOT_HPP
# define BOT_HPP

# include <string>

// ─────────────────────────────────────────────────────────────────────────────
//  PPBot — un cliente IRC independiente que se conecta al servidor ircserv
//  (o a cualquier servidor IRC de referencia) y responde al comando "ppsize"
//  con un "8====D" de longitud aleatoria seguido de una medida.
//
//  Uso:  ./ircbot <host> <port> <password> [nick]
//
//  - Responde tanto a PRIVMSG directos como a mensajes en un canal.
//  - Si lo invitan a un canal (INVITE), se une automáticamente.
//  - Responde a los PING del servidor con PONG.
// ─────────────────────────────────────────────────────────────────────────────

class Bot
{
	private:
		int			_fd;
		std::string	_host;
		int			_port;
		std::string	_password;
		std::string	_nick;
		std::string	_buffer;   // acumulador para troceado por líneas \r\n

		void	sendRaw(const std::string &msg) const;
		void	registerBot();
		void	handleLine(const std::string &line);
		void	onPrivmsg(const std::string &sender, const std::string &target,
						  const std::string &text);

		static std::string	makePpSize();

		Bot(const Bot &);
		Bot &operator=(const Bot &);

	public:
		Bot(const std::string &host, int port,
			const std::string &password, const std::string &nick);
		~Bot();

		void	connectToServer();
		void	run();
};

#endif
