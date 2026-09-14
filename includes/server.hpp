#ifndef SERVER_HPP
#define SERVER_HPP

#include "client.hpp"
#include "channel.hpp"
#include <map>
#include <string>
#include <sys/select.h>

struct Command;

class Server
{
	public:
		Server(int port, const std::string &password);
		~Server();

		bool start();
		bool run();

	private:
		typedef bool (Server::*CommandHandler)(Client &, const Command &);

		struct CommandEntry
		{
			const char *name;
			CommandHandler handler;
			bool requiresRegistration;
		};

		int _port;
		std::string _password;
		int _serverFd;
		int _maxFd;
		fd_set _master;
		std::map<int, Client> _clients;
		std::map<std::string, Channel> _channels;

		int createSocket() const;
		bool acceptClient();
		void disconnectClient(int fd);
		bool receiveFromClient(Client &client);
		void sendToClient(Client &client);
		bool reply(Client &client, const std::string &code,
			const std::string &parameters);
		bool sendLine(Client &client, const std::string &message);
		bool broadcast(Channel &channel, const std::string &message,
			int exceptFd, int senderFd);
		bool sendNames(Client &client, const Channel &channel);
		std::string clientPrefix(const Client &client) const;
		Channel *findChannel(const std::string &name);
		bool isNicknameAvailable(const std::string &nickname, int excludedFd) const;
		bool handleMessage(Client &client, const std::string &message);
		bool tryRegister(Client &client);
		bool handlePass(Client &client, const Command &command);
		bool handleNick(Client &client, const Command &command);
		bool handleUser(Client &client, const Command &command);
		bool handleJoin(Client &client, const Command &command);
		bool handlePart(Client &client, const Command &command);
		bool handlePrivmsg(Client &client, const Command &command);
		bool handleNotice(Client &client, const Command &command);
		bool handleQuit(Client &client, const Command &command);
		bool handlePing(Client &client, const Command &command);
		bool handlePong(Client &client, const Command &command);
		bool handleTopic(Client &client, const Command &command);
		bool handleInvite(Client &client, const Command &command);
		bool handleKick(Client &client, const Command &command);
		bool handleMode(Client &client, const Command &command);
		bool handleNames(Client &client, const Command &command);
		bool handleList(Client &client, const Command &command);
		bool handleWho(Client &client, const Command &command);
		bool handleWhois(Client &client, const Command &command);
		bool handleMotd(Client &client, const Command &command);

		// A Server owns its sockets and must not be copied.
		Server(const Server &other);
		Server &operator=(const Server &other);
};

#endif
