#include "server.hpp"
#include "parser.hpp"
#include <cctype>
#include <iostream>

bool Server::handleMessage(Client &client, const std::string &message)
{
	static const CommandEntry commands[] = {
		{"CAP", &Server::handleCap},
		{"PASS", &Server::handlePass},
		{"NICK", &Server::handleNick},
		{"USER", &Server::handleUser},
		{"JOIN", &Server::handleJoin},
		{"PART", &Server::handlePart},
		{"PRIVMSG", &Server::handlePrivmsg},
		{"NOTICE", &Server::handleNotice},
		{"QUIT", &Server::handleQuit},
		{"PING", &Server::handlePing},
		{"PONG", &Server::handlePong},
		{"TOPIC", &Server::handleTopic},
		{"INVITE", &Server::handleInvite},
		{"KICK", &Server::handleKick},
		{"MODE", &Server::handleMode},
		{"NAMES", &Server::handleNames},
		{"LIST", &Server::handleList},
		{"WHO", &Server::handleWho},
		{"WHOIS", &Server::handleWhois},
		{"MOTD", &Server::handleMotd}
	};

	Command command;
	if (!parseCommand(message, command))
		return true;
	for (std::size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i)
	{
		if (command.name == commands[i].name)
			return (this->*commands[i].handler)(client, command);
	}
	// Unknown commands are ignored until IRC error replies are implemented.
	return true;
}

bool Server::handleCap(Client &client, const Command &command)
{
	std::string subcommand;
	if (!command.params.empty())
		subcommand = command.params[0];
	for (std::size_t i = 0; i < subcommand.size(); ++i)
		subcommand[i] = std::toupper(static_cast<unsigned char>(subcommand[i]));

	if (subcommand == "LS")
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

// Placeholders: each command will get its own implementation.

bool Server::handlePass(Client &, const Command &)
{
	return true;
}

bool Server::handleNick(Client &, const Command &)
{
	return true;
}

bool Server::handleUser(Client &, const Command &)
{
	return true;
}

bool Server::handleJoin(Client &, const Command &)
{
	return true;
}

bool Server::handlePart(Client &, const Command &)
{
	return true;
}

bool Server::handlePrivmsg(Client &, const Command &)
{
	return true;
}

bool Server::handleNotice(Client &, const Command &)
{
	return true;
}

bool Server::handleQuit(Client &, const Command &)
{
	return true;
}

bool Server::handlePing(Client &, const Command &)
{
	return true;
}

bool Server::handlePong(Client &, const Command &)
{
	return true;
}

bool Server::handleTopic(Client &, const Command &)
{
	return true;
}

bool Server::handleInvite(Client &, const Command &)
{
	return true;
}

bool Server::handleKick(Client &, const Command &)
{
	return true;
}

bool Server::handleMode(Client &, const Command &)
{
	return true;
}

bool Server::handleNames(Client &, const Command &)
{
	return true;
}

bool Server::handleList(Client &, const Command &)
{
	return true;
}

bool Server::handleWho(Client &, const Command &)
{
	return true;
}

bool Server::handleWhois(Client &, const Command &)
{
	return true;
}

bool Server::handleMotd(Client &, const Command &)
{
	return true;
}
