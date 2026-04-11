#include <sstream>
#include "irc.hpp"

IrcMessage::IrcMessage() {}

bool	IrcMessage::parse(const std::string &line, IrcMessage &out); // TODO

std::string	IrcMessage::toString() const;