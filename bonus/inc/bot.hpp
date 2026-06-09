#pragma once

#include <ctime>
#include <algorithm>
#include <iostream>


class Bot
{
	private:
		std::string	_nick;
		std::string	_user;
		std::string	_host;

		void		sendMsg(std::string &msg) const;

	public:
		Bot();
		// Bot(std::string host, std::string pass, )
		~Bot();

		void	botConnect();
		void	update();
};