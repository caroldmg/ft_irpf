#ifndef IRCMESSAGE_HPP
# define IRCMESSAGE_HPP

# include <string>
# include <vector>

// Mensaje IRC parseado: [:prefix] COMMAND [param1 ... [:trailing]]
struct IrcMessage
{
	std::string					prefix;
	std::string					command;
	std::vector<std::string>	params;

	IrcMessage();

	// Parsea una linea IRC sin el \r\n final. Devuelve false si la linea esta vacia.
	static bool	parse(const std::string &line, IrcMessage &out);

	// Reconstruye el mensaje (debug/log).
	std::string	toString() const;
};

#endif
