#include "iniLoader.h"

void iniLoader::loadIni(INI_STRUCT* ini, std::string path) {
	std::map<std::string, std::map<std::string, std::string>> section_keyVaule;
	std::ifstream file(path);
	std::string line;
	std::string contextSection;
	if (file.is_open()) {
		while (std::getline(file, line)) {
			line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
			if (line[0] == '[') {
				std::string section = line.substr(1, line.length() - 2);
				std::map<std::string, std::string> key_value;
				section_keyVaule.insert(std::make_pair(section, key_value));
				contextSection = section;
			}

			if (line.find("=") != std::string::npos) {
				std::string key, value;
				key = line.substr(0, line.find("="));
				value = line.substr(line.find("=") + 1, line.length() - line.find("=") - 1);
				auto it = section_keyVaule.find(contextSection);
				it->second.insert(std::make_pair(key, value));
			}

		}
		file.close();
	}
	*ini = section_keyVaule;
}

void iniLoader::editKey(INI_STRUCT* ini, std::string section, std::string key, std::string value)
{
	if ((*ini).find(section) == (*ini).end()) {
		std::map<std::string, std::string> key_value;
		(*ini).insert(std::make_pair(section, key_value));
	}
	else {
		if ((*ini).find(section)->second.find(key) == (*ini).find(section)->second.end()) {
			(*ini).find(section)->second.insert(std::make_pair(key, value));
		}
		else {
			(*ini).find(section)->second.find(key)->second = value;
		}
	}
}

void iniLoader::deleteKey(INI_STRUCT* ini, std::string section, std::string key)
{
	auto it_section = (*ini).find(section);
	if (it_section != (*ini).end()) {
		auto it_key = it_section->second.find(key);
		if (it_key != it_section->second.end()) {
			it_section->second.erase(key);
			if (it_section->second.empty() == true) {
				(*ini).erase(section);
			}
		}
	}
}

std::string iniLoader::readKey(INI_STRUCT ini, std::string section, std::string key)
{
	auto it_section = ini.find(section);
	if (it_section != ini.end()) {
		auto it_key = it_section->second.find(key);
		if (it_key != it_section->second.end()) {
			return it_key->second;
		}
	}
	return std::string();
}

void iniLoader::writeIni(INI_STRUCT ini, std::string path)
{
	std::ofstream outfile(path);
	std::string iniContent = std::string();
	for (const auto& section_keyValue : ini) {
		iniContent += "[" + section_keyValue.first + "]" + "\n";
		for (const auto& key_value : section_keyValue.second) {
			iniContent += key_value.first + "=" + key_value.second + "\n";
		}
		iniContent += "\n";
	}
	outfile << iniContent;
	outfile.close();
}
