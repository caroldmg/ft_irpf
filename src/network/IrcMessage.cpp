#include <sstream>
#include "IrcMessage.hpp"

IrcMessage::IrcMessage() {}

bool	IrcMessage::parse(const std::string &line, IrcMessage &out)
{
	out.prefix.clear();
	out.command.clear();
	out.params.clear();

	if (line.empty())
		return (false);

	size_t	i = 0;

	// prefix opcional
	if (line[0] == ':')
	{
		size_t sp = line.find(' ', 1);
		if (sp == std::string::npos)
			return (false);
		out.prefix = line.substr(1, sp - 1);
		i = sp + 1;
		while (i < line.size() && line[i] == ' ')
			++i;
	}

	// command
	size_t start = i;
	while (i < line.size() && line[i] != ' ')
		++i;
	if (start == i)
		return (false);
	out.command = line.substr(start, i - start);
	for (size_t k = 0; k < out.command.size(); ++k)
		if (out.command[k] >= 'a' && out.command[k] <= 'z')
			out.command[k] = static_cast<char>(out.command[k] - 'a' + 'A');

	// params
	while (i < line.size())
	{
		while (i < line.size() && line[i] == ' ')
			++i;
		if (i >= line.size())
			break;

		if (line[i] == ':')
		{
			out.params.push_back(line.substr(i + 1));
			break;
		}

		size_t pstart = i;
		while (i < line.size() && line[i] != ' ')
			++i;
		out.params.push_back(line.substr(pstart, i - pstart));
	}

	return (!out.command.empty());
}

std::string	IrcMessage::toString() const
{
	std::string out;
	if (!prefix.empty())
		out += ":" + prefix + " ";
	out += command;
	for (size_t i = 0; i < params.size(); ++i)
	{
		out += " ";
		if (i + 1 == params.size() && (params[i].find(' ') != std::string::npos || params[i].empty() || params[i][0] == ':'))
			out += ":" + params[i];
		else
			out += params[i];
	}
	return (out);
}
