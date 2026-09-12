#ifndef SERVER_HPP
#define SERVER_HPP

#include "client.hpp"
#include <map>
#include <string>
#include <sys/select.h>

class Server
{
	public:
		Server(int port, const std::string &password);
		~Server();

		bool start();
		bool run();

	private:
		int _port;
		std::string _password;
		int _serverFd;
		int _maxFd;
		fd_set _master;
		std::map<int, Client> _clients;

		int createSocket() const;
		bool acceptClient();
		void disconnectClient(int fd);
		bool receiveFromClient(Client &client);
		void sendToClient(Client &client);
		bool handleMessage(Client &client, const std::string &message);

		// A Server owns its sockets and must not be copied.
		Server(const Server &other);
		Server &operator=(const Server &other);
};

#endif
