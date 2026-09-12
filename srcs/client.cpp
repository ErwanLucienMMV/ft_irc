#include "client.hpp"

static const std::size_t MAX_INPUT_SIZE = 8192;
static const std::size_t MAX_OUTPUT_SIZE = 65536;

Client::Client(int socketFd, const std::string &ip, unsigned short remotePort)
	: _fd(socketFd), _address(ip), _port(remotePort), _readClosed(false),
	_ioFailures(0), _passwordAccepted(false), _registered(false),
	_capNegotiating(false)
{
}

int Client::getFd() const
{
	return _fd;
}

const std::string &Client::getAddress() const
{
	return _address;
}

unsigned short Client::getPort() const
{
	return _port;
}

const std::string &Client::getNickname() const
{
	return _nickname;
}

const std::string &Client::getUsername() const
{
	return _username;
}

const std::string &Client::getRealname() const
{
	return _realname;
}

bool Client::isPasswordAccepted() const
{
	return _passwordAccepted;
}

bool Client::isRegistered() const
{
	return _registered;
}

bool Client::isCapNegotiating() const
{
	return _capNegotiating;
}


void Client::setNickname(const std::string &nickname)
{
	_nickname = nickname;
}

void Client::setUserInfo(const std::string &username, const std::string &realname)
{
	_username = username;
	_realname = realname;
}

void Client::setPasswordAccepted(bool accepted)
{
	_passwordAccepted = accepted;
}

void Client::setCapNegotiating(bool negotiating)
{
	_capNegotiating = negotiating;
}

void Client::markRegistered()
{
	_registered = true;
}


bool Client::appendReceived(const char *data, std::size_t size)
{
	if (size > MAX_INPUT_SIZE - _input.size())
		return false;
	_input.append(data, size);
	return true;
}

bool Client::extractLine(std::string &line)
{
	std::string::size_type end = _input.find('\n');

	if (end == std::string::npos)
		return false;

	line = _input.substr(0, end);

	_input.erase(0, end + 1);
	if (!line.empty() && line[line.size() - 1] == '\r')
		line.erase(line.size() - 1);

	return true;
}

bool Client::hasIncompleteLineTooLong() const
{
	// A full IRC line fits in 512 bytes, including CRLF.
	return _input.size() > 511;
}

bool Client::queueMessage(const std::string &message)
{
	if (message.size() > MAX_OUTPUT_SIZE - _output.size())
		return false;
	_output += message;
	return true;
}

const std::string &Client::getPendingOutput() const
{
	return _output;
}

void Client::consumeOutput(std::size_t size)
{
	_output.erase(0, size);
}

bool Client::hasPendingOutput() const
{
	return !_output.empty();
}

void Client::closeRead()
{
	_readClosed = true;
}

bool Client::isReadClosed() const
{
	return _readClosed;
}

bool Client::recordIoResult(bool success)
{
	if (success)
		_ioFailures = 0;
	else
		++_ioFailures;
	// Bound retries on readiness events without inspecting errno after I/O.
	return _ioFailures < 3;
}
