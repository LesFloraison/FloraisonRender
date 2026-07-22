#pragma once

#include "iniLoader.h"

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

class MRenderCore;

enum class RuntimeActionType {
	Exit,
	ReloadScene,
	ReloadInterface
};

struct RuntimeAction {
	RuntimeActionType type;
	std::string argument;
};

struct ExecutorContext {
	std::function<MRenderCore*()> renderer;
	INI_STRUCT* config = nullptr;
};

struct ExecutionSummary {
	std::size_t executed = 0;
	std::vector<std::string> errors;

	bool success() const { return errors.empty(); }
};

void initializeExecutor(ExecutorContext context);
void shutdownExecutor();

void enqueueCommand(std::string command, std::string source = "runtime", std::size_t line = 0);
std::vector<RuntimeAction> takeRuntimeActions();

void consoleInput();
void consoleProcess();
ExecutionSummary executeSingle(std::string_view command, std::string source = "internal", std::size_t line = 0);
ExecutionSummary executeScript(const std::string& scriptPath);
