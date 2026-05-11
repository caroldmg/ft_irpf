#include <unistd.h>
#include <sys/socket.h>
#include "Client.hpp"

// Constructor
Client::Client()
	: _fd(-1), _passOk(false), _hasNick(false), _hasUser(false) {}

Client::Client(int fd)
	: _fd(fd), _passOk(false), _hasNick(false), _hasUser(false) {}

Client::Client(const Client &o)
	: _fd(o._fd), _nick(o._nick), _user(o._user), _host(o._host),
	  _realname(o._realname), _buffer(o._buffer),
	  _passOk(o._passOk), _hasNick(o._hasNick), _hasUser(o._hasUser) {}

Client &Client::operator=(const Client &o)
{
	if (this != &o)
	{
		_fd = o._fd;
		_nick = o._nick;
		_user = o._user;
		_host = o._host;
		_realname = o._realname;
		_buffer = o._buffer;
		_passOk = o._passOk;
		_hasNick = o._hasNick;
		_hasUser = o._hasUser;
	}
	return (*this);
}

Client::~Client() {}

// Getter
int					Client::getFd()        const { return (_fd); }
const std::string	&Client::getNick()     const { return (_nick); }
const std::string	&Client::getUser()     const { return (_user); }
const std::string	&Client::getHost()     const { return (_host); }
const std::string	&Client::getRealname() const { return (_realname); }
bool				Client::hasPassOk()    const { return (_passOk); }
bool				Client::hasNick()      const { return (_hasNick); }
bool				Client::hasUser()      const { return (_hasUser); }


// Bool atribute checkers
bool	Client::isRegistered() const
{
	return (_passOk && _hasNick && _hasUser);
}


// Setters
void	Client::setNick(const std::string &nick)        { _nick = nick; _hasNick = true; }
void	Client::setUser(const std::string &user)        { _user = user; _hasUser = true; }
void	Client::setHost(const std::string &host)        { _host = host; }
void	Client::setRealname(const std::string &r)       { _realname = r; }
void	Client::setPassOk(bool ok)                      { _passOk = ok; }


// functions
void	Client::appendToBuffer(const std::string &data) { _buffer += data; }

bool	Client::hasCompleteLine() const
{
	return (_buffer.find('\n') != std::string::npos);
}

std::string	Client::flushLine()
{
	size_t pos = _buffer.find('\n');
	if (pos == std::string::npos)
		return ("");
	std::string line = _buffer.substr(0, pos);
	_buffer.erase(0, pos + 1);
	if (!line.empty() && line[line.size() - 1] == '\r')
		line.erase(line.size() - 1);
	return (line);
}

void	Client::sendMsg(const std::string &msg) const
{
	if (_fd >= 0)
		send(_fd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
}

std::string	Client::getPrefix() const
{
	return (_nick + "!" + _user + "@" + _host);
}
