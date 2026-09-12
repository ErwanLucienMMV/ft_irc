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
		void run();

	private:
		int _port;
		std::string _password;
		int _serverFd;
		int _maxFd;
		fd_set _master;
		std::map<int, Client> _clients;

		int createSocket() const;
		void acceptClient();
		void disconnectClient(int fd);
		void receiveFromClient(const Client &client);
		void handleMessage(const Client &client, const std::string &message);

		// A Server owns its sockets and must not be copied.
		Server(const Server &other);
		Server &operator=(const Server &other);
};

#endif
