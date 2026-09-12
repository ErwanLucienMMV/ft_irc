#include "client.hpp"

Client::Client(int socketFd, const std::string &ip, unsigned short remotePort)
	: _fd(socketFd), _address(ip), _port(remotePort)
{
}

int Client::getFd() const
{
	return _fd;
}

const std::string &Client::getAddress() const
{
	return _address;
}

unsigned short Client::getPort() const
{
	return _port;
}
