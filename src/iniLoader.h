#pragma once
#include <map>
#include <string>
#include <fstream>
#include <algorithm>
#include <iostream>
#define INI_STRUCT std::map<std::string, std::map<std::string, std::string>>

namespace iniLoader {
	void loadIni(INI_STRUCT* ini, std::string);
	void writeIni(INI_STRUCT ini, std::string path);
	void editKey(INI_STRUCT* ini, std::string section, std::string key, std::string value);
	void deleteKey(INI_STRUCT* ini, std::string section, std::string key);
	std::string readKey(INI_STRUCT ini, std::string section, std::string key);
}
