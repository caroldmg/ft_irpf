#ifndef IRC_HPP
# define IRC_HPP
 
# include <string>
# include <vector>
 
// mensaje IRC ya parseado.
// [:prefix] COMMAND [param1 param2 ... [:trailing]]
struct IrcMessage
{
	std::string					prefix;   
	std::string					command;  
	std::vector<std::string>	params;   
 
	IrcMessage();
 
	// Parsea una linea IRC completa sin el \r\n final.
	static bool	parse(const std::string &line, IrcMessage &out);
 
	// Reconstruye el mensaje como string legible (util para logs y debug).
	std::string	toString() const;
};


#ifndef REPLIES_HPP
# define REPLIES_HPP

# include <string>

// TODO: Hay que meter las macros que generan respuestas por defecto IRC ya formateadas con \r\n al final.
// Formato general: ":ircserv <codigo> <nick_destino> <contenido>\r\n"