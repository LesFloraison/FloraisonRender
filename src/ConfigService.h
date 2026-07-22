#pragma once

#include "iniLoader.h"

#include <string>

bool loadConfig(INI_STRUCT& config, const std::string& path, std::string& error);
