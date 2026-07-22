#include "CommandParser.h"

#include <cctype>
#include <iterator>

namespace command {
namespace {

std::string_view trim(std::string_view value)
{
	while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
		value.remove_prefix(1);
	}
	while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
		value.remove_suffix(1);
	}
	return value;
}

bool isComment(std::string_view value)
{
	value = trim(value);
	return value.size() >= 2 && value[0] == '/' && value[1] == '/';
}

bool parseSegment(std::string_view segment, std::size_t segmentColumn, ParseOutput& output)
{
	segment = trim(segment);
	if (segment.empty()) {
		return false;
	}
	if (isComment(segment)) {
		return true;
	}

	std::vector<std::string> tokens;
	std::string token;
	bool tokenStarted = false;
	bool inQuotes = false;

	for (std::size_t i = 0; i < segment.size(); ++i) {
		const char current = segment[i];
		if (current == '"') {
			inQuotes = !inQuotes;
			tokenStarted = true;
			continue;
		}

		if (inQuotes && current == '\\' && i + 1 < segment.size()
			&& (segment[i + 1] == '"' || segment[i + 1] == '\\')) {
			token.push_back(segment[++i]);
			tokenStarted = true;
			continue;
		}

		if (!inQuotes && std::isspace(static_cast<unsigned char>(current))) {
			if (tokenStarted) {
				tokens.push_back(std::move(token));
				token.clear();
				tokenStarted = false;
			}
			continue;
		}

		token.push_back(current);
		tokenStarted = true;
	}

	if (inQuotes) {
		output.issues.push_back({ segmentColumn, "unterminated quoted argument" });
		return false;
	}
	if (tokenStarted) {
		tokens.push_back(std::move(token));
	}
	if (tokens.empty()) {
		return false;
	}

	ParsedCommand parsed;
	parsed.name = std::move(tokens.front());
	parsed.arguments.assign(
		std::make_move_iterator(tokens.begin() + 1),
		std::make_move_iterator(tokens.end()));
	parsed.text.assign(segment.begin(), segment.end());
	output.commands.push_back(std::move(parsed));
	return false;
}

} // namespace

ParseOutput parseProgram(std::string_view input)
{
	ParseOutput output;
	std::size_t segmentStart = 0;
	bool inQuotes = false;

	for (std::size_t i = 0; i < input.size(); ++i) {
		const char current = input[i];
		if (current == '"') {
			bool escaped = false;
			std::size_t slash = i;
			while (slash > segmentStart && input[slash - 1] == '\\') {
				--slash;
			}
			escaped = ((i - slash) % 2) != 0;
			if (!escaped) {
				inQuotes = !inQuotes;
			}
		}
		else if (current == ';' && !inQuotes) {
			if (parseSegment(input.substr(segmentStart, i - segmentStart), segmentStart, output)) {
				return output;
			}
			segmentStart = i + 1;
		}
	}

	parseSegment(input.substr(segmentStart), segmentStart, output);
	return output;
}

} // namespace command
