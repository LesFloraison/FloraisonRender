#include "ConfigService.h"

#include "encapVk.h"

#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <utility>

namespace {

bool readInteger(const INI_STRUCT& config, const std::string& section, const std::string& key,
	int minimum, int maximum, int& value, std::string& error)
{
	const std::string text = iniLoader::readKey(config, section, key);
	const char* begin = text.data();
	const char* end = begin + text.size();
	const auto result = std::from_chars(begin, end, value);
	if (text.empty() || result.ec != std::errc() || result.ptr != end
		|| value < minimum || value > maximum) {
		error = "invalid configuration value [" + section + "]." + key + ": " + text;
		return false;
	}
	return true;
}

bool readFloat(const INI_STRUCT& config, const std::string& section, const std::string& key,
	float minimum, float maximum, float& value, std::string& error)
{
	const std::string text = iniLoader::readKey(config, section, key);
	errno = 0;
	char* end = nullptr;
	value = std::strtof(text.c_str(), &end);
	if (text.empty() || errno == ERANGE || end != text.c_str() + text.size()
		|| !std::isfinite(value) || value < minimum || value > maximum) {
		error = "invalid configuration value [" + section + "]." + key + ": " + text;
		return false;
	}
	return true;
}

} // namespace

bool loadConfig(INI_STRUCT& config, const std::string& path, std::string& error)
{
	std::error_code fileError;
	if (!std::filesystem::is_regular_file(path, fileError)) {
		error = "configuration file does not exist: " + path;
		return false;
	}

	INI_STRUCT loaded;
	iniLoader::loadIni(&loaded, path);

	int fullScreen = 0;
	int innerWidth = 0;
	int innerHeight = 0;
	int outerWidth = 0;
	int outerHeight = 0;
	int radianceCacheRadius = 0;
	int primarySamples = 0;
	int secondarySamples = 0;
	int fsr = 0;
	int taau = 0;
	int infiniteDiffuse = 0;
	float nearPlane = 0.0f;
	float farPlane = 0.0f;
	float fov = 0.0f;

	if (!readInteger(loaded, "general", "full_screen", 0, 1, fullScreen, error)
		|| !readInteger(loaded, "general", "inner_width", 1, 32768, innerWidth, error)
		|| !readInteger(loaded, "general", "inner_height", 1, 32768, innerHeight, error)
		|| !readInteger(loaded, "general", "outer_width", 1, 32768, outerWidth, error)
		|| !readInteger(loaded, "general", "outer_height", 1, 32768, outerHeight, error)
		|| !readFloat(loaded, "general", "near_plane", 0.0001f, 1000000.0f, nearPlane, error)
		|| !readFloat(loaded, "general", "far_plane", 0.0001f, 1000000.0f, farPlane, error)
		|| !readFloat(loaded, "general", "fov", 1.0f, 179.0f, fov, error)
		|| !readInteger(loaded, "graphic", "radiance_cache_rad", 1, 32768, radianceCacheRadius, error)
		|| !readInteger(loaded, "graphic", "ssp_1", 1, 4096, primarySamples, error)
		|| !readInteger(loaded, "graphic", "ssp_2", 1, 4096, secondarySamples, error)
		|| !readInteger(loaded, "graphic", "fsr", 0, 1, fsr, error)
		|| !readInteger(loaded, "graphic", "taau", 0, 1, taau, error)
		|| !readInteger(loaded, "graphic", "inf_diffuse", 0, 1, infiniteDiffuse, error)) {
		return false;
	}
	if (nearPlane >= farPlane) {
		error = "near_plane must be smaller than far_plane";
		return false;
	}

	config = std::move(loaded);
	FULL_SCREEN = fullScreen;
	INNER_WIDTH = innerWidth;
	INNER_HEIGHT = innerHeight;
	OUTER_WIDTH = outerWidth;
	OUTER_HEIGHT = outerHeight;
	NEAR_PLANE = nearPlane;
	FAR_PLANE = farPlane;
	FOV = fov;
	RADIANCE_CACHE_RAD = radianceCacheRadius;
	SSP = primarySamples;
	SSP_2 = secondarySamples;
	FSR = fsr;
	UIEnable = taau;
	debugVal = infiniteDiffuse;
	return true;
}
