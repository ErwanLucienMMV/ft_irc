#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

// Connection data only: Server is responsible for closing the socket.
class Client
{
	public:
		Client(int socketFd, const std::string &ip, unsigned short remotePort);

		int getFd() const;
		const std::string &getAddress() const;
		unsigned short getPort() const;

	private:
		int _fd;
		std::string _address;
		unsigned short _port;
};

#endif
