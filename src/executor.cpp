#include "Executor.h"

#include "CommandParser.h"
#include "MCameraTrack.h"
#include "MInterface.h"
#include "MPipeline.h"
#include "MRenderCore.h"
#include "encapVk.h"

#include <GLFW/glfw3.h>

#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace {

struct CommandOutcome {
	std::string error;
	std::optional<RuntimeAction> action;
};

using CommandHandler = std::function<CommandOutcome(const command::ParsedCommand&)>;

struct CommandSpec {
	std::size_t minimumArguments = 0;
	std::size_t maximumArguments = 0;
	std::string usage;
	CommandHandler handler;
};

struct QueuedCommand {
	std::string command;
	std::string source;
	std::size_t line = 0;
};

struct ExecutorState {
	ExecutorContext context;
	std::unordered_map<std::string, CommandSpec> commands;
	std::deque<QueuedCommand> pendingCommands;
	std::vector<RuntimeAction> runtimeActions;
	std::unique_ptr<MCameraTrack> cameraTrack;
	std::mutex queueMutex;
	bool initialized = false;
};

ExecutorState state;

std::string location(const std::string& source, std::size_t line)
{
	if (line == 0) {
		return source;
	}
	return source + ":" + std::to_string(line);
}

CommandOutcome failure(std::string message)
{
	return { std::move(message), std::nullopt };
}

CommandOutcome success()
{
	return {};
}

CommandOutcome action(RuntimeActionType type, std::string argument = {})
{
	return { {}, RuntimeAction{ type, std::move(argument) } };
}

bool parseInt(const std::string& text, int& value)
{
	if (text.empty()) {
		return false;
	}
	const char* begin = text.data();
	const char* end = begin + text.size();
	const auto result = std::from_chars(begin, end, value);
	return result.ec == std::errc() && result.ptr == end;
}

bool parseFloat(const std::string& text, float& value)
{
	if (text.empty()) {
		return false;
	}
	errno = 0;
	char* end = nullptr;
	value = std::strtof(text.c_str(), &end);
	return errno != ERANGE && end == text.c_str() + text.size() && std::isfinite(value);
}

CommandOutcome integerArgument(
	const command::ParsedCommand& parsed,
	int minimum,
	int maximum,
	const std::function<void(int)>& apply)
{
	int value = 0;
	if (!parseInt(parsed.arguments[0], value)) {
		return failure("argument must be an integer");
	}
	if (value < minimum || value > maximum) {
		return failure("argument must be in range [" + std::to_string(minimum) + ", "
			+ std::to_string(maximum) + "]");
	}
	apply(value);
	return success();
}

CommandOutcome floatArgument(
	const command::ParsedCommand& parsed,
	float minimum,
	float maximum,
	const std::function<void(float)>& apply)
{
	float value = 0.0f;
	if (!parseFloat(parsed.arguments[0], value)) {
		return failure("argument must be a finite number");
	}
	if (value < minimum || value > maximum) {
		std::ostringstream message;
		message << "argument must be in range [" << minimum << ", " << maximum << ']';
		return failure(message.str());
	}
	apply(value);
	return success();
}

MRenderCore* renderer()
{
	return state.context.renderer ? state.context.renderer() : nullptr;
}

CommandOutcome requireFile(const command::ParsedCommand& parsed, RuntimeActionType type)
{
	std::error_code error;
	const std::filesystem::path path(parsed.arguments[0]);
	if (!std::filesystem::is_regular_file(path, error)) {
		return failure("file does not exist or is not a regular file: " + path.string());
	}
	return action(type, path.string());
}

void addCommand(std::string name, std::size_t minimumArguments, std::size_t maximumArguments,
	std::string usage, CommandHandler handler)
{
	state.commands.emplace(std::move(name), CommandSpec{
		minimumArguments, maximumArguments, std::move(usage), std::move(handler)
	});
}

void addConfigInteger(const std::string& name, const std::string& section, const std::string& key,
	int minimum, int maximum)
{
	addCommand(name, 1, 1, name + " <integer>", [section, key, minimum, maximum](const auto& parsed) {
		if (state.context.config == nullptr) {
			return failure("configuration service is not available");
		}
		return integerArgument(parsed, minimum, maximum, [section, key](int value) {
			iniLoader::editKey(state.context.config, section, key, std::to_string(value));
		});
	});
}

void addConfigFloat(const std::string& name, const std::string& section, const std::string& key,
	float minimum, float maximum)
{
	addCommand(name, 1, 1, name + " <number>", [section, key, minimum, maximum](const auto& parsed) {
		if (state.context.config == nullptr) {
			return failure("configuration service is not available");
		}
		return floatArgument(parsed, minimum, maximum, [section, key](float value) {
			iniLoader::editKey(state.context.config, section, key, std::to_string(value));
		});
	});
}

int configuredInteger(const std::string& section, const std::string& key, int fallback)
{
	if (state.context.config == nullptr) {
		return fallback;
	}
	int value = fallback;
	if (!parseInt(iniLoader::readKey(*state.context.config, section, key), value)) {
		return fallback;
	}
	return value;
}

void updateDerivedInnerResolution()
{
	if (state.context.config == nullptr) {
		return;
	}
	const int outerWidth = configuredInteger("general", "outer_width", OUTER_WIDTH);
	const int outerHeight = configuredInteger("general", "outer_height", OUTER_HEIGHT);
	const int fsrEnabled = configuredInteger("graphic", "fsr", FSR);
	const float divisor = fsrEnabled != 0 ? 2.5f : 2.0f;
	iniLoader::editKey(state.context.config, "general", "inner_width",
		std::to_string(static_cast<int>(outerWidth / divisor)));
	iniLoader::editKey(state.context.config, "general", "inner_height",
		std::to_string(static_cast<int>(outerHeight / divisor)));
}

void registerCommands()
{
	state.commands.clear();

	addCommand("exit", 0, 0, "exit", [](const auto&) {
		return action(RuntimeActionType::Exit);
	});

	addCommand("cursor_mode", 1, 1, "cursor_mode <0|1>", [](const auto& parsed) {
		return integerArgument(parsed, 0, 1, [](int value) {
			glfwSetInputMode(window, GLFW_CURSOR,
				value == 0 ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
		});
	});

	addCommand("speed", 1, 1, "speed <number>", [](const auto& parsed) {
		return floatArgument(parsed, 0.0f, 1000.0f, [](float value) { cameraSpeed = value; });
	});

	addCommand("load_scene", 1, 1, "load_scene <path>", [](const auto& parsed) {
		return requireFile(parsed, RuntimeActionType::ReloadScene);
	});
	addCommand("load_interface", 1, 1, "load_interface <path>", [](const auto& parsed) {
		return requireFile(parsed, RuntimeActionType::ReloadInterface);
	});

	addCommand("camera_track", 1, 1, "camera_track <path>", [](const auto& parsed) {
		std::error_code fileError;
		if (!std::filesystem::is_regular_file(parsed.arguments[0], fileError)) {
			return failure("camera track file does not exist: " + parsed.arguments[0]);
		}
		if (MCameraTrack::isTracking.load()) {
			return failure("a camera track is already running");
		}
		state.cameraTrack.reset();
		state.cameraTrack = std::make_unique<MCameraTrack>();
		std::string error;
		if (!state.cameraTrack->traceDecode(parsed.arguments[0], error)
			|| !state.cameraTrack->beginExecute(error)) {
			state.cameraTrack.reset();
			return failure(error);
		}
		return success();
	});

	addCommand("freecam", 1, 1, "freecam <0|1>", [](const auto& parsed) {
		return integerArgument(parsed, 0, 1, [](int value) { freeCam = value != 0; });
	});
	addCommand("gbuffer", 1, 1, "gbuffer <-1..64>", [](const auto& parsed) {
		return integerArgument(parsed, -1, 64, [](int value) { displayID = value; });
	});
	addCommand("taau", 1, 1, "taau <0|1>", [](const auto& parsed) {
		return integerArgument(parsed, 0, 1, [](int value) { UIEnable = value; });
	});
	addCommand("inf_diffuse", 1, 1, "inf_diffuse <0|1>", [](const auto& parsed) {
		return integerArgument(parsed, 0, 1, [](int value) { debugVal = value; });
	});
	addCommand("interface_page", 1, 1, "interface_page <page>", [](const auto& parsed) {
		return integerArgument(parsed, 0, 1024, [](int value) { MInterface::page = value; });
	});

	addCommand("text_disable", 1, 1, "text_disable <id>", [](const auto& parsed) {
		return integerArgument(parsed, 0,
			static_cast<int>(MInterface::textDisableTable.size()) - 1,
			[](int value) { MInterface::textDisableTable[static_cast<std::size_t>(value)] = 1; });
	});
	addCommand("text_enable", 1, 1, "text_enable <id>", [](const auto& parsed) {
		return integerArgument(parsed, 0,
			static_cast<int>(MInterface::textDisableTable.size()) - 1,
			[](int value) { MInterface::textDisableTable[static_cast<std::size_t>(value)] = 0; });
	});

	addCommand("write_interface_state", 0, 0, "write_interface_state", [](const auto&) {
		MRenderCore* core = renderer();
		if (core == nullptr || core->p_interface == nullptr) {
			return failure("renderer interface is not available in the current phase");
		}
		core->p_interface->writeStateFile();
		return success();
	});
	addCommand("restart_check", 0, 0, "restart_check", [](const auto&) {
		MRenderCore* core = renderer();
		if (core == nullptr || core->p_interface == nullptr) {
			return failure("renderer interface is not available in the current phase");
		}
		core->p_interface->RestartCheck();
		return success();
	});

	addConfigInteger("config_full_screen", "general", "full_screen", 0, 1);
	addConfigInteger("config_inner_width", "general", "inner_width", 1, 32768);
	addConfigInteger("config_inner_height", "general", "inner_height", 1, 32768);
	addCommand("config_outer_width", 1, 1, "config_outer_width <integer>", [](const auto& parsed) {
		if (state.context.config == nullptr) {
			return failure("configuration service is not available");
		}
		return integerArgument(parsed, 1, 32768, [](int value) {
			iniLoader::editKey(state.context.config, "general", "outer_width", std::to_string(value));
			updateDerivedInnerResolution();
		});
	});
	addCommand("config_outer_height", 1, 1, "config_outer_height <integer>", [](const auto& parsed) {
		if (state.context.config == nullptr) {
			return failure("configuration service is not available");
		}
		return integerArgument(parsed, 1, 32768, [](int value) {
			iniLoader::editKey(state.context.config, "general", "outer_height", std::to_string(value));
			updateDerivedInnerResolution();
		});
	});
	addConfigFloat("config_near_plane", "general", "near_plane", 0.0001f, 1000000.0f);
	addConfigFloat("config_far_plane", "general", "far_plane", 0.0001f, 1000000.0f);
	addConfigFloat("config_fov", "general", "fov", 1.0f, 179.0f);
	addConfigInteger("config_radiance_cache_rad", "graphic", "radiance_cache_rad", 1, 32768);
	addConfigInteger("config_ssp_1", "graphic", "ssp_1", 1, 4096);
	addConfigInteger("config_ssp_2", "graphic", "ssp_2", 1, 4096);
	addCommand("config_fsr", 1, 1, "config_fsr <0|1>", [](const auto& parsed) {
		if (state.context.config == nullptr) {
			return failure("configuration service is not available");
		}
		return integerArgument(parsed, 0, 1, [](int value) {
			iniLoader::editKey(state.context.config, "graphic", "fsr", std::to_string(value));
			updateDerivedInnerResolution();
		});
	});

	addCommand("save_config", 0, 0, "save_config", [](const auto&) {
		if (state.context.config == nullptr) {
			return failure("configuration service is not available");
		}
		iniLoader::writeIni(*state.context.config, "res/config/cfg.ini");
		return success();
	});

	addCommand("shader_recompile", 0, 1, "shader_recompile [all]", [](const auto& parsed) {
		if (parsed.arguments.empty()) {
			MPipeline::shaderRecompile("shaders");
			return success();
		}
		if (parsed.arguments[0] != "all") {
			return failure("usage: shader_recompile [all]");
		}
		MPipeline::shaderRecompile("shaders", true);
		return success();
	});
}

} // namespace

void initializeExecutor(ExecutorContext context)
{
	state.context = std::move(context);
	registerCommands();
	state.initialized = true;
}

void shutdownExecutor()
{
	state.cameraTrack.reset();
	{
		std::lock_guard<std::mutex> lock(state.queueMutex);
		state.pendingCommands.clear();
	}
	state.runtimeActions.clear();
	state.commands.clear();
	state.context = {};
	state.initialized = false;
}

void enqueueCommand(std::string command, std::string source, std::size_t line)
{
	std::lock_guard<std::mutex> lock(state.queueMutex);
	state.pendingCommands.push_back({ std::move(command), std::move(source), line });
}

std::vector<RuntimeAction> takeRuntimeActions()
{
	std::vector<RuntimeAction> actions;
	actions.swap(state.runtimeActions);
	return actions;
}

void consoleInput()
{
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	std::string input;
	if (std::getline(std::cin, input)) {
		enqueueCommand(std::move(input), "console");
	}
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void consoleProcess()
{
	std::deque<QueuedCommand> commands;
	{
		std::lock_guard<std::mutex> lock(state.queueMutex);
		commands.swap(state.pendingCommands);
	}
	for (const QueuedCommand& queued : commands) {
		executeSingle(queued.command, queued.source, queued.line);
	}
}

ExecutionSummary executeSingle(std::string_view input, std::string source, std::size_t line)
{
	ExecutionSummary summary;
	if (!state.initialized) {
		summary.errors.push_back("executor has not been initialized");
		return summary;
	}

	command::ParseOutput parsed = command::parseProgram(input);
	for (const command::ParseIssue& issue : parsed.issues) {
		summary.errors.push_back("column " + std::to_string(issue.column + 1) + ": " + issue.message);
	}

	for (const command::ParsedCommand& current : parsed.commands) {
		const auto found = state.commands.find(current.name);
		if (found == state.commands.end()) {
			summary.errors.push_back("unknown command \"" + current.name + "\"");
			continue;
		}

		const CommandSpec& spec = found->second;
		if (current.arguments.size() < spec.minimumArguments
			|| current.arguments.size() > spec.maximumArguments) {
			summary.errors.push_back("usage: " + spec.usage);
			continue;
		}

		try {
			CommandOutcome outcome = spec.handler(current);
			if (!outcome.error.empty()) {
				summary.errors.push_back(current.name + ": " + outcome.error);
				continue;
			}
			if (outcome.action.has_value()) {
				state.runtimeActions.push_back(std::move(*outcome.action));
			}
			++summary.executed;
		}
		catch (const std::exception& error) {
			summary.errors.push_back(current.name + ": " + error.what());
		}
	}

	for (const std::string& error : summary.errors) {
		std::cerr << location(source, line) << ": " << error << std::endl;
	}
	return summary;
}

ExecutionSummary executeScript(const std::string& scriptPath)
{
	ExecutionSummary total;
	std::ifstream file(scriptPath);
	if (!file.is_open()) {
		total.errors.push_back("could not open script: " + scriptPath);
		std::cerr << total.errors.front() << std::endl;
		return total;
	}

	std::string line;
	std::size_t lineNumber = 0;
	while (std::getline(file, line)) {
		++lineNumber;
		ExecutionSummary current = executeSingle(line, scriptPath, lineNumber);
		total.executed += current.executed;
		total.errors.insert(total.errors.end(),
			std::make_move_iterator(current.errors.begin()),
			std::make_move_iterator(current.errors.end()));
	}
	return total;
}
