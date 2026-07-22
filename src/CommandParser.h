#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace command {

struct ParsedCommand {
	std::string name;
	std::vector<std::string> arguments;
	std::string text;
};

struct ParseIssue {
	std::size_t column = 0;
	std::string message;
};

struct ParseOutput {
	std::vector<ParsedCommand> commands;
	std::vector<ParseIssue> issues;
};

ParseOutput parseProgram(std::string_view input);

} // namespace command
