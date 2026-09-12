#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

// Connection data only: Server is responsible for closing the socket.
class Client
{
	public:
		Client(int socketFd, const std::string &ip, unsigned short remotePort);

		int getFd() const;
		const std::string &getAddress() const;
		unsigned short getPort() const;

		const std::string &getNickname() const;
		const std::string &getUsername() const;
		const std::string &getRealname() const;
		bool isPasswordAccepted() const;
		bool isRegistered() const;
		bool isCapNegotiating() const;

		void setNickname(const std::string &nickname);
		void setUserInfo(const std::string &username, const std::string &realname);
		void setPasswordAccepted(bool accepted);
		void setCapNegotiating(bool negotiating);
		void markRegistered();

		bool appendReceived(const char *data, std::size_t size);
		bool extractLine(std::string &line);
		bool hasIncompleteLineTooLong() const;
		bool queueMessage(const std::string &message);
		const std::string &getPendingOutput() const;
		void consumeOutput(std::size_t size);
		bool hasPendingOutput() const;
		void closeRead();
		bool isReadClosed() const;
		bool recordIoResult(bool success);

	private:
		int _fd;
		std::string _address;
		unsigned short _port;
		std::string _input;
		std::string _output;
		bool _readClosed;
		unsigned int _ioFailures;
		std::string _nickname;
		std::string _username;
		std::string _realname;
		bool _passwordAccepted;
		bool _registered;
		bool _capNegotiating;
};

#endif
