#include "CommandParser.h"

#include <cassert>
#include <iostream>

int main()
{
	{
		const auto output = command::parseProgram("  speed 3.5; gbuffer 16; ");
		assert(output.issues.empty());
		assert(output.commands.size() == 2);
		assert(output.commands[0].name == "speed");
		assert(output.commands[0].arguments == std::vector<std::string>{ "3.5" });
		assert(output.commands[1].name == "gbuffer");
	}
	{
		const auto output = command::parseProgram("// ignored; exit");
		assert(output.issues.empty());
		assert(output.commands.empty());
	}
	{
		const auto output = command::parseProgram("gbuffer 16; // stop here; exit");
		assert(output.issues.empty());
		assert(output.commands.size() == 1);
		assert(output.commands[0].name == "gbuffer");
	}
	{
		const auto output = command::parseProgram("load_scene \"a scene;01.txt\"");
		assert(output.issues.empty());
		assert(output.commands.size() == 1);
		assert(output.commands[0].arguments == std::vector<std::string>{ "a scene;01.txt" });
	}
	{
		const auto output = command::parseProgram("load_scene \"unterminated");
		assert(output.commands.empty());
		assert(output.issues.size() == 1);
	}
	{
		const auto output = command::parseProgram(";;  ;\t");
		assert(output.commands.empty());
		assert(output.issues.empty());
	}
	{
		const auto output = command::parseProgram("exit_now");
		assert(output.commands.size() == 1);
		assert(output.commands[0].name == "exit_now");
	}

	std::cout << "CommandParser tests passed" << std::endl;
}
