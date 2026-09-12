#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

// Connection data only: Server is responsible for closing the socket.
struct Client
{
	int fd;
	std::string address;
	unsigned short port;

	Client(int socketFd, const std::string &ip, unsigned short remotePort)
		: fd(socketFd), address(ip), port(remotePort)
	{
	}
};

#endif
