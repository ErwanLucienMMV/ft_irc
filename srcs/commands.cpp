#include "server.hpp"
#include "parser.hpp"
#include <cstdlib>
#include <vector>

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

static std::string foldName(std::string name)
{
	for (std::size_t i = 0; i < name.size(); ++i)
	{
		if (name[i] >= 'A' && name[i] <= 'Z')
			name[i] += 'a' - 'A';
		else if (name[i] == '[') name[i] = '{';
		else if (name[i] == ']') name[i] = '}';
		else if (name[i] == '\\') name[i] = '|';
		else if (name[i] == '^') name[i] = '~';
	}
	return name;
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
	return sendLine(client, ":localhost " + code + " " + target + " " + parameters);
}

bool Server::sendLine(Client &client, const std::string &text)
{
	std::string message = text;
	if (message.size() > 510)
		message.resize(510);
	return client.queueMessage(message + "\r\n");
}

std::string Server::clientPrefix(const Client &client) const
{
	return ":" + client.getNickname() + "!" + client.getUsername()
		+ "@" + client.getAddress();
}

Channel *Server::findChannel(const std::string &name)
{
	std::map<std::string, Channel>::iterator it = _channels.find(foldName(name));
	if (it == _channels.end())
		return NULL;
	return &it->second;
}

bool Server::broadcast(Channel &channel, const std::string &message,
	int exceptFd, int senderFd)
{
	std::set<int> members = channel.getMembers();
	std::vector<int> failed;
	bool senderFailed = false;
	for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
	{
		if (*it == exceptFd)
			continue;
		std::map<int, Client>::iterator client = _clients.find(*it);
		if (client != _clients.end() && !sendLine(client->second, message))
			failed.push_back(*it);
	}
	for (std::vector<int>::const_iterator it = failed.begin(); it != failed.end(); ++it)
	{
		if (*it == senderFd)
			senderFailed = true;
		else
			disconnectClient(*it);
	}
	return !senderFailed;
}

bool Server::sendNames(Client &client, const Channel &channel)
{
	std::string names;
	for (std::set<int>::const_iterator it = channel.getMembers().begin();
		it != channel.getMembers().end(); ++it)
	{
		std::map<int, Client>::const_iterator member = _clients.find(*it);
		if (member == _clients.end())
			continue;
		if (!names.empty())
			names += " ";
		if (channel.isOperator(*it))
			names += "@";
		names += member->second.getNickname();
	}
	if (!reply(client, "353", "= " + channel.getName() + " :" + names))
		return false;
	return reply(client, "366", channel.getName() + " :End of /NAMES list");
}

bool Server::isNicknameAvailable(const std::string &nickname, int excludedFd) const
{
	const std::string folded = foldName(nickname);
	for (std::map<int, Client>::const_iterator it = _clients.begin();
		it != _clients.end(); ++it)
	{
		if (it->first != excludedFd
			&& foldName(it->second.getNickname()) == folded)
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

static bool isValidChannelName(const std::string &name)
{
	if (name.empty() || name.size() > 50 || (name[0] != '#' && name[0] != '&'))
		return false;
	return name.find_first_of(" ,:\a\r\n") == std::string::npos;
}

bool Server::handleJoin(Client &client, const Command &command)
{
	if (command.params.empty())
		return reply(client, "461", "JOIN :Not enough parameters");
	const std::string &name = command.params[0];
	if (!isValidChannelName(name))
		return reply(client, "403", name + " :No such channel");

	Channel *channel = findChannel(name);
	if (channel == NULL)
	{
		std::pair<std::map<std::string, Channel>::iterator, bool> inserted =
			_channels.insert(std::make_pair(foldName(name), Channel(name, client.getFd())));
		channel = &inserted.first->second;
	}
	else
	{
		if (channel->hasMember(client.getFd()))
			return true;
		if (channel->isInviteOnly() && !channel->isInvited(client.getFd()))
			return reply(client, "473", channel->getName() + " :Cannot join channel (+i)");
		if (!channel->getKey().empty()
			&& (command.params.size() < 2 || command.params[1] != channel->getKey()))
			return reply(client, "475", channel->getName() + " :Cannot join channel (+k)");
		if (channel->isFull())
			return reply(client, "471", channel->getName() + " :Cannot join channel (+l)");
		channel->addMember(client.getFd());
		channel->removeInvite(client.getFd());
	}

	if (!broadcast(*channel, clientPrefix(client) + " JOIN :" + channel->getName(),
		-1, client.getFd()))
		return false;
	if (channel->getTopic().empty())
	{
		if (!reply(client, "331", channel->getName() + " :No topic is set"))
			return false;
	}
	else if (!reply(client, "332", channel->getName() + " :" + channel->getTopic()))
		return false;
	return sendNames(client, *channel);
}

bool Server::handlePart(Client &client, const Command &command)
{
	if (command.params.empty())
		return reply(client, "461", "PART :Not enough parameters");
	Channel *channel = findChannel(command.params[0]);
	if (channel == NULL)
		return reply(client, "403", command.params[0] + " :No such channel");
	if (!channel->hasMember(client.getFd()))
		return reply(client, "442", channel->getName() + " :You're not on that channel");

	std::string part = clientPrefix(client) + " PART " + channel->getName();
	if (command.params.size() > 1)
		part += " :" + command.params[1];
	if (!broadcast(*channel, part, -1, client.getFd()))
		return false;
	channel->removeMember(client.getFd());
	if (channel->isEmpty())
		_channels.erase(foldName(channel->getName()));
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

bool Server::handleNames(Client &client, const Command &command)
{
	if (!command.params.empty())
	{
		Channel *channel = findChannel(command.params[0]);
		if (channel != NULL)
			return sendNames(client, *channel);
		return reply(client, "366", command.params[0] + " :End of /NAMES list");
	}
	for (std::map<std::string, Channel>::const_iterator it = _channels.begin();
		it != _channels.end(); ++it)
	{
		if (!sendNames(client, it->second))
			return false;
	}
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
