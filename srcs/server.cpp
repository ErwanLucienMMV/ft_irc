#include "server.hpp"
#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define BUFFER_SIZE 4096

int createServer(int port)
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
	addr.sin_port = htons(port);

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

static void acceptClient(int server_fd, fd_set &master, int &max_fd)
{
	sockaddr_in client;
	socklen_t len = sizeof(client);
	int client_fd = accept(server_fd, (sockaddr *)&client, &len);

	if (client_fd < 0)
		return;

	FD_SET(client_fd, &master);
	if (client_fd > max_fd)
		max_fd = client_fd;

	std::cout << "Client connected: "
		<< inet_ntoa(client.sin_addr)
		<< ":" << ntohs(client.sin_port)
		<< " (fd " << client_fd << ")"
		<< std::endl;
}

static void disconnectClient(int fd, fd_set &master)
{
	std::cout << "Client disconnected (fd " << fd << ")" << std::endl;
	close(fd);
	FD_CLR(fd, &master);
}

static void receiveFromClient(int fd, fd_set &master)
{
	char buffer[BUFFER_SIZE];
	int bytes = recv(fd, buffer, sizeof(buffer), 0);

	if (bytes <= 0)
	{
		disconnectClient(fd, master);
		return;
	}

	std::cout << "----- RECEIVED " << bytes << " BYTES -----" << std::endl;
	// Print exactly the received bytes: the buffer may not end with '\0'.
	std::cout.write(buffer, bytes);
	std::cout << std::endl;
	std::cout << "--------------------------------" << std::endl;
}

void runServer(int server_fd)
{
	fd_set master;
	fd_set readfds;

	FD_ZERO(&master);
	FD_SET(server_fd, &master);
	int max_fd = server_fd;

	while (true)
	{
		readfds = master;
		if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0)
			break;

		for (int fd = 0; fd <= max_fd; ++fd)
		{
			if (!FD_ISSET(fd, &readfds))
				continue;

			if (fd == server_fd)
				acceptClient(server_fd, master, max_fd);
			else
				receiveFromClient(fd, master);
		}
	}
}
