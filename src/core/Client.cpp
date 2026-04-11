#include <unistd.h>
#include <sys/socket.h>
#include "Client.hpp"


Client::Client()
	: _fd(-1), _passOk(false), _hasNick(false), _hasUser(false)
{}

Client::Client(int fd)
	: _fd(fd), _passOk(false), _hasNick(false), _hasUser(false)
{}

Client::Client(const Client &other)
	: _fd(other._fd),
	  _nick(other._nick),
	  _user(other._user),
	  _host(other._host),
	  _realname(other._realname),
	  _buffer(other._buffer),
	  _passOk(other._passOk),
	  _hasNick(other._hasNick),
	  _hasUser(other._hasUser)
{}

Client &Client::operator=(const Client &other)
{
	if (this != &other)
	{
		_fd       = other._fd;
		_nick     = other._nick;
		_user     = other._user;
		_host     = other._host;
		_realname = other._realname;
		_buffer   = other._buffer;
		_passOk   = other._passOk;
		_hasNick  = other._hasNick;
		_hasUser  = other._hasUser;
	}
	return *this;
}

Client::~Client() {}

//  getters 

int					Client::getFd()        const { return _fd; }
const std::string	&Client::getNick()     const { return _nick; }
const std::string	&Client::getUser()     const { return _user; }
const std::string	&Client::getHost()     const { return _host; }
const std::string	&Client::getRealname() const { return _realname; }
bool				Client::hasPassOk()    const { return _passOk; }
bool				Client::hasNick()      const { return _hasNick; }
bool				Client::hasUser()      const { return _hasUser; }

bool	Client::isRegistered() const
{
	return (_passOk && _hasNick && _hasUser);
}

//  setters 

void	Client::setNick(const std::string &nick)
{
	_nick    = nick;
	_hasNick = true;
}

void	Client::setUser(const std::string &user)
{
	_user    = user;
	_hasUser = true;
}

void	Client::setHost(const std::string &host)     { _host = host; }
void	Client::setRealname(const std::string &r)    { _realname = r; }
void	Client::setPassOk(bool ok)                   { _passOk = ok; }


void	Client::appendToBuffer(const std::string &data)
{
	_buffer += data;
}

bool	Client::hasCompleteLine() const
{
	return (_buffer.find("\r\n") != std::string::npos);
}

std::string	Client::flushLine()
{
	size_t pos = _buffer.find("\r\n");
	if (pos == std::string::npos)
		return "";

	std::string line = _buffer.substr(0, pos);
	_buffer.erase(0, pos + 2); // elimina la linea y el \r\n del buffer
	return line;
}


void	Client::sendMsg(const std::string &msg) const
{
	send(_fd, msg.c_str(), msg.size(), 0);
}


std::string	Client::getPrefix() const
{
	// formato: nick!user@host 
	return (_nick + "!" + _user + "@" + _host);
}