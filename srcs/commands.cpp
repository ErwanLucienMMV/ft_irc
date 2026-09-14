#include "server.hpp"
#include "parser.hpp"

bool Server::handleMessage(Client &client, const std::string &message)
{
	static const CommandEntry commands[] = {
		{"PASS", &Server::handlePass, false},
		{"NICK", &Server::handleNick, false},
		{"USER", &Server::handleUser, false},
		{"JOIN", &Server::handleJoin, true},
		{"PART", &Server::handlePart, true},
		{"PRIVMSG", &Server::handlePrivmsg, true},
		{"NOTICE", &Server::handleNotice, true},
		{"QUIT", &Server::handleQuit, false},
		{"PING", &Server::handlePing, false},
		{"PONG", &Server::handlePong, false},
		{"TOPIC", &Server::handleTopic, true},
		{"INVITE", &Server::handleInvite, true},
		{"KICK", &Server::handleKick, true},
		{"MODE", &Server::handleMode, true},
		{"NAMES", &Server::handleNames, true},
		{"LIST", &Server::handleList, true},
		{"WHO", &Server::handleWho, true},
		{"WHOIS", &Server::handleWhois, true},
		{"MOTD", &Server::handleMotd, true}
	};

	Command command;
	if (!parseCommand(message, command))
		return true;
	for (std::size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i)
	{
		if (command.name == commands[i].name)
		{
			if (commands[i].requiresRegistration && !client.isRegistered())
				return reply(client, "451", ":You have not registered");
			return (this->*commands[i].handler)(client, command);
		}
	}
	return reply(client, "421", command.name + " :Unknown command");
}

static std::string foldNickname(std::string nickname)
{
	for (std::size_t i = 0; i < nickname.size(); ++i)
	{
		if (nickname[i] >= 'A' && nickname[i] <= 'Z')
			nickname[i] += 'a' - 'A';
		else if (nickname[i] == '[') nickname[i] = '{';
		else if (nickname[i] == ']') nickname[i] = '}';
		else if (nickname[i] == '\\') nickname[i] = '|';
		else if (nickname[i] == '^') nickname[i] = '~';
	}
	return nickname;
}

static bool isValidNickname(const std::string &nickname)
{
	static const std::string first =
		"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ[]\\`_^{|}~";
	static const std::string rest = first + "0123456789-";
	// Local nickname limit; advertise it when adding RPL_ISUPPORT.
	return !nickname.empty() && nickname.size() <= 30
		&& first.find(nickname[0]) != std::string::npos
		&& nickname.find_first_not_of(rest) == std::string::npos;
}

static bool isValidUsername(const std::string &username)
{
	if (username.empty())
		return false;
	for (std::size_t i = 0; i < username.size(); ++i)
	{
		unsigned char c = static_cast<unsigned char>(username[i]);
		if (c <= ' ' || c == 127 || c == '@' || c == '!' || c == ':')
			return false;
	}
	return true;
}

bool Server::reply(Client &client, const std::string &code,
	const std::string &parameters)
{
	std::string target = client.getNickname();
	if (target.empty())
		target = "*";
	std::string message = ":localhost " + code + " " + target + " " + parameters;
	if (message.size() > 510)
		message.resize(510);
	return client.queueMessage(message + "\r\n");
}

bool Server::isNicknameAvailable(const std::string &nickname, int excludedFd) const
{
	const std::string folded = foldNickname(nickname);
	for (std::map<int, Client>::const_iterator it = _clients.begin();
		it != _clients.end(); ++it)
	{
		if (it->first != excludedFd
			&& foldNickname(it->second.getNickname()) == folded)
			return false;
	}
	return true;
}

bool Server::tryRegister(Client &client)
{
	if (client.isRegistered() || !client.isPasswordAccepted()
		|| client.getNickname().empty() || client.getUsername().empty())
		return true;

	if (!reply(client, "001", ":Welcome to the IRC Network"))
		return false;
	client.markRegistered();
	return true;
}

bool Server::handlePass(Client &client, const Command &command)
{
	if (client.isRegistered())
		return reply(client, "462", ":You may not reregister");
	if (command.params.size() != 1)
		return reply(client, "461", "PASS :Not enough parameters");

	client.setPasswordAccepted(_password.empty() || _password == command.params[0]);
	if (!client.isPasswordAccepted())
		return reply(client, "464", ":Password incorrect");
	return tryRegister(client);
}

bool Server::handleNick(Client &client, const Command &command)
{
	if (command.params.size() != 1 || command.params[0].empty())
		return reply(client, "431", ":No nickname given");
	const std::string &nickname = command.params[0];
	if (!isValidNickname(nickname))
		// Do not insert malformed client text into a middle reply parameter.
		return reply(client, "432", "* :Erroneous nickname");
	if (!isNicknameAvailable(nickname, client.getFd()))
		return reply(client, "433", nickname + " :Nickname is already in use");
	client.setNickname(nickname);
	return tryRegister(client);
}

bool Server::handleUser(Client &client, const Command &command)
{
	if (client.isRegistered() || !client.getUsername().empty())
		return reply(client, "462", ":You may not reregister");
	if (command.params.size() != 4)
		return reply(client, "461", "USER :Not enough parameters");
	if (!isValidUsername(command.params[0]))
		return reply(client, "461", "USER :Invalid username");
	client.setUserInfo(command.params[0], command.params[3]);
	return tryRegister(client);
}

// Remaining commands will get their own implementation.

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
	return false;
}

bool Server::handlePing(Client &client, const Command &command)
{
	if (command.params.empty() || command.params[0].empty())
		return reply(client, "409", ":No origin specified");
	return client.queueMessage("PONG :" + command.params[0] + "\r\n");
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
