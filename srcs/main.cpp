#include <iostream>
#include <exception>
#include <string>
#include "server.hpp"

static int parsePort(const char *text)
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
		return -1;
	return port;
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

	if (!*argv[2])
	{
		std::cerr << "Error: password cannot be empty" << std::endl;
		return 1;
	}
	try
	{
		Server server(port, argv[2]);
		if (!server.start() || !server.run())
			return 1;
	}
	catch (const std::exception &error)
	{
		std::cerr << "Error: " << error.what() << std::endl;
		return 1;
	}

	return 0;
}
