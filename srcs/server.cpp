#include "server.hpp"
#include <iostream>
#include <cstdio>
#include <cstring>
#include <utility>
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <csignal>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define BUFFER_SIZE 4096

static volatile sig_atomic_t stopRequested = 0;

static void requestStop(int)
{
	stopRequested = 1;
}

static bool configureSignals()
{
	struct sigaction action;
	std::memset(&action, 0, sizeof(action));
	sigemptyset(&action.sa_mask);
	action.sa_handler = requestStop;
	if (sigaction(SIGINT, &action, NULL) < 0
		|| sigaction(SIGTERM, &action, NULL) < 0)
	{
		std::perror("sigaction");
		return false;
	}
	action.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &action, NULL) < 0)
	{
		std::perror("sigaction");
		return false;
	}
	stopRequested = 0;
	return true;
}

static bool configureSocket(int fd)
{
	if (fd >= FD_SETSIZE)
	{
		std::cerr << "Error: socket exceeds select capacity." << std::endl;
		return false;
	}
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
	{
		std::perror("fcntl");
		return false;
	}
	return true;
}

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
	if (!configureSocket(server_fd))
	{
		close(server_fd);
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
	if (!configureSignals())
		return false;
	_serverFd = createSocket();
	if (_serverFd < 0)
		return false;
	FD_SET(_serverFd, &_master);
	_maxFd = _serverFd;
	std::cout << "Listening on port " << _port << std::endl;
	return true;
}

bool Server::acceptClient()
{
	sockaddr_in client;
	socklen_t len = sizeof(client);
	int client_fd = accept(_serverFd, (sockaddr *)&client, &len);

	if (client_fd < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK
			|| errno == EINTR || errno == ECONNABORTED)
			return true;
		std::perror("accept");
		return false;
	}
	if (!configureSocket(client_fd))
	{
		close(client_fd);
		return true;
	}

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
	return true;
}

void Server::disconnectClient(int fd)
{
	std::cout << "Client disconnected (fd " << fd << ")" << std::endl;
	close(fd);
	FD_CLR(fd, &_master);
	_clients.erase(fd);
	_maxFd = _serverFd;
	if (!_clients.empty())
		_maxFd = std::max(_maxFd, _clients.rbegin()->first);
}

bool Server::receiveFromClient(Client &client)
{
	char buffer[BUFFER_SIZE];
	ssize_t bytes = recv(client.getFd(), buffer, sizeof(buffer), 0);

	if (!client.recordIoResult(bytes >= 0))
	{
		disconnectClient(client.getFd());
		return false;
	}
	if (bytes < 0)
		return true;
	if (bytes == 0)
	{
		client.closeRead();
		if (!client.hasPendingOutput())
		{
			disconnectClient(client.getFd());
			return false;
		}
		return true;
	}
	std::cout << "----- RECEIVED " << bytes << " BYTES -----" << std::endl;
	// Print exactly the received bytes: the buffer may not end with '\0'.
	std::cout.write(buffer, bytes);
	std::cout << std::endl;
	std::cout << "--------------------------------" << std::endl;

	if (!client.appendReceived(buffer, static_cast<std::size_t>(bytes)))
	{
		disconnectClient(client.getFd());
		return false;
	}
	std::string line;
	while (client.extractLine(line))
	{
		if (line.size() > 510 || !handleMessage(client, line))
		{
			disconnectClient(client.getFd());
			return false;
		}
	}
	if (client.hasIncompleteLineTooLong())
	{
		disconnectClient(client.getFd());
		return false;
	}
	return true;
}

void Server::sendToClient(Client &client)
{
	const std::string &output = client.getPendingOutput();
	std::size_t size = std::min(output.size(), static_cast<std::size_t>(BUFFER_SIZE));
	ssize_t bytes = send(client.getFd(), output.data(), size, 0);
	if (!client.recordIoResult(bytes > 0))
	{
		disconnectClient(client.getFd());
		return;
	}
	if (bytes <= 0)
		return;
	client.consumeOutput(static_cast<std::size_t>(bytes));
	if (!client.hasPendingOutput() && client.isReadClosed())
		disconnectClient(client.getFd());
}

bool Server::handleMessage(Client &client, const std::string &message)
{
	// Only CAP LS is implemented here; registration will be added later.
	std::istringstream input(message);
	std::string command;
	std::string subcommand;
	input >> command >> subcommand;
	for (std::size_t i = 0; i < command.size(); ++i)
		command[i] = std::toupper(static_cast<unsigned char>(command[i]));
	for (std::size_t i = 0; i < subcommand.size(); ++i)
		subcommand[i] = std::toupper(static_cast<unsigned char>(subcommand[i]));

	if (command == "CAP" && subcommand == "LS")
	{
		const char *response = ":localhost CAP * LS :\r\n";
		if (!client.queueMessage(response))
			return false;
		std::cout << "----- QUEUED -----" << std::endl;
		std::cout << response;
		std::cout << "------------------" << std::endl;
	}
	return true;
}

bool Server::run()
{
	if (_serverFd < 0)
		return false;

	while (!stopRequested)
	{
		fd_set readfds = _master;
		fd_set writefds;
		FD_ZERO(&writefds);
		for (std::map<int, Client>::const_iterator it = _clients.begin();
			it != _clients.end(); ++it)
		{
			if (it->second.isReadClosed())
				FD_CLR(it->first, &readfds);
			if (it->second.hasPendingOutput())
				FD_SET(it->first, &writefds);
		}

		// The timeout also handles a stop signal arriving just before select().
		timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if (select(_maxFd + 1, &readfds, &writefds, NULL, &timeout) < 0)
		{
			if (errno == EINTR)
				continue;
			std::perror("select");
			return false;
		}

		for (int fd = 0; fd <= _maxFd && !stopRequested; ++fd)
		{
			if (!FD_ISSET(fd, &readfds) && !FD_ISSET(fd, &writefds))
				continue;

			if (fd == _serverFd)
			{
				if (!acceptClient())
					return false;
				continue;
			}
			Client &client = _clients.find(fd)->second;
			if (FD_ISSET(fd, &readfds) && !receiveFromClient(client))
				continue;
			if (FD_ISSET(fd, &writefds))
				sendToClient(client);
		}
	}
	return true;
}
