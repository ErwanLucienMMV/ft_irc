#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include "server.hpp"

int parsePort(const char *text)
{
    int port = 0;

    if (!*text)
        return -1;
    while (*text)
    {
        if (*text < '0' || *text > '9')
            return -1;
        port = port * 10 + (*text - '0');
        if (port > 65535)
            return -1;
        ++text;
    }
	if (port == 0)
		return (-1);
	return (port);
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
		return 1;
	}

	int port = parsePort(argv[1]);
	if (port < 0)
	{
		std::cerr << "Error: port must be an integer between 1 and 65535" << std::endl;
		return 1;
	}

	std::string password = argv[2];
	if (password.empty())
	{
		std::cerr << "Warning: No password has been set, this is a safety issue, consider it carefully" << std::endl;
		return 1;
	}

	int server_fd = createServer(port);
	if (server_fd < 0)
		return 1;
	std::cout << "Listening on port " << port << std::endl;
	runServer(server_fd);
	close(server_fd);

	return 0;
}
