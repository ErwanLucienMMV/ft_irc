#include <iostream>
#include <cstring>
#include <cstdlib>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define BUFFER_SIZE 4096

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
		return 1;
	}

	int port = std::atoi(argv[1]);

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
		return 1;

	int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0)
		return 1;

	if (listen(server_fd, SOMAXCONN) < 0)
		return 1;

	std::cout << "Listening on port " << port << std::endl;

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

			/* New connection */
			if (fd == server_fd)
			{
				sockaddr_in client;
				socklen_t len = sizeof(client);

				int client_fd = accept(
					server_fd,
					(sockaddr *)&client,
					&len
				);

				if (client_fd < 0)
					continue;

				FD_SET(client_fd, &master);

				if (client_fd > max_fd)
					max_fd = client_fd;

				std::cout << "Client connected: "
					<< inet_ntoa(client.sin_addr)
					<< ":" << ntohs(client.sin_port)
					<< " (fd " << client_fd << ")"
					<< std::endl;
			}
			/* Existing client sent data */
			else
			{
				char buffer[BUFFER_SIZE];

				int bytes = recv(fd, buffer, sizeof(buffer), 0);

				if (bytes <= 0)
				{
					std::cout << "Client disconnected (fd "
						<< fd << ")" << std::endl;

					close(fd);
					FD_CLR(fd, &master);
					continue;
				}

				std::cout << "----- RECEIVED "
					<< bytes << " BYTES -----" << std::endl;

				/*
				 * Print EXACTLY what was received.
				 *
				 * std::cout.write() is important:
				 * it does not expect a '\0' at the end.
				 */
				std::cout.write(buffer, bytes);

				std::cout << std::endl;
				std::cout << "--------------------------------"
					<< std::endl;
			}
		}
	}

	close(server_fd);
	return 0;
}