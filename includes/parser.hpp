#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>

struct Command
{
	std::string name;
	std::vector<std::string> params;
};

// Client line without CRLF, at most 510 bytes and 15 parameters (RFC 2812).
// Prefixes and IRCv3 tags are unsupported; false leaves command empty.
// Command-specific parameter checks belong to the handlers.
bool parseCommand(const std::string &line, Command &command);

#endif
