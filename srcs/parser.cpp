#include "parser.hpp"
#include <cctype>

static bool isValidCommandName(const std::string &name)
{
	if (name.empty())
		return false;
	bool letters = true;
	bool digits = name.size() == 3;
	for (std::size_t i = 0; i < name.size(); ++i)
	{
		if (name[i] < 'A' || name[i] > 'Z')
			letters = false;
		if (name[i] < '0' || name[i] > '9')
			digits = false;
	}
	return letters || digits;
}

bool parseCommand(const std::string &line, Command &command)
{
	command.name.clear();
	command.params.clear();
	if (line.size() > 510 || line.find_first_of("\r\n") != std::string::npos
		|| line.find('\0') != std::string::npos)
		return false;
	Command parsed;
	std::size_t i = 0;

	while (i < line.size() && line[i] == ' ')
		++i;
	while (i < line.size() && line[i] != ' ')
	{
		parsed.name += std::toupper(static_cast<unsigned char>(line[i]));
		++i;
	}

	if (!isValidCommandName(parsed.name))
		return false;

	while (i < line.size())
	{
		while (i < line.size() && line[i] == ' ')
			++i;
		if (i == line.size())
			break;

		if (line[i] == ':' || parsed.params.size() == 14)
		{
			if (line[i] == ':')
				++i;
			parsed.params.push_back(line.substr(i));
			break;
		}
		std::size_t start = i;
		while (i < line.size() && line[i] != ' ')
			++i;
		parsed.params.push_back(line.substr(start, i - start));
	}
	command = parsed;
	return true;
}
