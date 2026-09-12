#include "server.hpp"
#include <iostream>
#include <cstdio>
#include <cstring>
#include <utility>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define BUFFER_SIZE 4096

Server::Server(int port, const std::string &password)
	: _port(port), _password(password), _serverFd(-1), _maxFd(-1)
{
	FD_ZERO(&_master);
}

Server::~Server()
{
	for (std::map<int, Client>::const_iterator it = _clients.begin();
		it != _clients.end(); ++it)
		close(it->second.getFd());
	if (_serverFd >= 0)
		close(_serverFd);
}

int Server::createSocket() const
{
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
	{
		std::perror("socket");
		return -1;
	}

	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		std::perror("setsockopt");
		close(server_fd);
		return -1;
	}

	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(_port);

	if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0)
	{
		std::perror("bind");
		close(server_fd);
		return -1;
	}

	if (listen(server_fd, SOMAXCONN) < 0)
	{
		std::perror("listen");
		close(server_fd);
		return -1;
	}

	return server_fd;
}

bool Server::start()
{
	if (_serverFd >= 0)
		return true;
	_serverFd = createSocket();
	if (_serverFd < 0)
		return false;
	FD_SET(_serverFd, &_master);
	_maxFd = _serverFd;
	std::cout << "Listening on port " << _port << std::endl;
	return true;
}

void Server::acceptClient()
{
	sockaddr_in client;
	socklen_t len = sizeof(client);
	int client_fd = accept(_serverFd, (sockaddr *)&client, &len);

	if (client_fd < 0)
		return;

	try
	{
		_clients.insert(std::make_pair(client_fd,
			Client(client_fd, inet_ntoa(client.sin_addr), ntohs(client.sin_port))));
	}
	catch (...)
	{
		close(client_fd);
		throw;
	}

	FD_SET(client_fd, &_master);
	if (client_fd > _maxFd)
		_maxFd = client_fd;

	const Client &connected = _clients.find(client_fd)->second;

	std::cout << "Client connected: "
		<< connected.getAddress()
		<< ":" << connected.getPort()
		<< " (fd " << connected.getFd() << ")"
		<< std::endl;
}

void Server::disconnectClient(int fd)
{
	std::cout << "Client disconnected (fd " << fd << ")" << std::endl;
	close(fd);
	FD_CLR(fd, &_master);
	_clients.erase(fd);
}

void Server::receiveFromClient(const Client &client)
{
	char buffer[BUFFER_SIZE];
	int bytes = recv(client.getFd(), buffer, sizeof(buffer), 0);

	if (bytes <= 0)
	{
		disconnectClient(client.getFd());
		return;
	}
	std::cout << "----- RECEIVED " << bytes << " BYTES -----" << std::endl;
	// Print exactly the received bytes: the buffer may not end with '\0'.
	std::cout.write(buffer, bytes);
	std::cout << std::endl;
	std::cout << "--------------------------------" << std::endl;

	handleMessage(client, std::string(buffer, bytes));
}

void Server::handleMessage(const Client &client, const std::string &message)
{
	// Temporary CAP handling; a line parser will replace this substring search.
	if (message.find("CAP LS") != std::string::npos)
	{
		const char *response = ":localhost CAP * LS :\r\n";

		std::cout << "----- SENDING -----" << std::endl;
		std::cout << response;
		std::cout << "-------------------" << std::endl;

		send(client.getFd(), response, std::strlen(response), 0);
	}
}
void Server::run()
{
	fd_set readfds;

	while (true)
	{
		readfds = _master;
		if (select(_maxFd + 1, &readfds, NULL, NULL, NULL) < 0)
			break;

		for (int fd = 0; fd <= _maxFd; ++fd)
		{
			if (!FD_ISSET(fd, &readfds))
				continue;

			if (fd == _serverFd)
				acceptClient();
			else
				receiveFromClient(_clients.find(fd)->second);
		}
	}
}
