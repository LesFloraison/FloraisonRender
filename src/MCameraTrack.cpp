#include "MCameraTrack.h"

#include <algorithm>
#include <functional>
#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>

std::atomic_bool MCameraTrack::isTracking{ false };
glm::vec3 MCameraTrack::MCTcameraDirection;
glm::vec3 MCameraTrack::MCTinvCameraPos;

void MCameraTrack::traceSampling(float requestedSamplingHz, float requestedMaxSecond, std::string path)
{
	glm::vec3 initialPosition = -invCameraPos;
	while (!stopRequested.load() && initialPosition == -invCameraPos) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	const auto start = std::chrono::high_resolution_clock::now();
	while (isRecording.load() && !stopRequested.load()) {
		traceStream.push_back(-invCameraPos.x);
		traceStream.push_back(-invCameraPos.y);
		traceStream.push_back(-invCameraPos.z);
		traceStream.push_back(cameraDirection.x);
		traceStream.push_back(cameraDirection.y);
		traceStream.push_back(cameraDirection.z);
		std::this_thread::sleep_for(
			std::chrono::milliseconds(static_cast<int>(1000.0f / requestedSamplingHz)));
		std::cout << "recording" << std::endl;
		if (traceStream.size() > requestedMaxSecond * requestedSamplingHz * 6) {
			isRecording.store(false);
		}
	}

	if (!stopRequested.load()) {
		std::ofstream outfile(path);
		outfile << requestedSamplingHz << ',' << requestedMaxSecond << ',';
		for (float value : traceStream) {
			outfile << value << ',';
		}
	}

	const auto duration = std::chrono::duration_cast<std::chrono::seconds>(
		std::chrono::high_resolution_clock::now() - start);
	std::cout << "recordingTime: " << duration.count() << "s" << std::endl;
}

void MCameraTrack::traceExecuting()
{
	std::vector<float> deltaTimeValues;
	freeCam = true;
	const auto start = std::chrono::high_resolution_clock::now();
	MCTinvCameraPos = -tracePositionStream[0];
	MCTcameraDirection = traceDirectionStream[0];
	const float samplingTime = 1000.0f / samplingHz;
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	for (std::size_t i = 0; i < tracePositionStream.size() && !stopRequested.load(); ++i) {
		float accumulatedFrameTime = 0.0f;
		auto loopEnd = std::chrono::high_resolution_clock::now();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		const glm::vec3 beginPosition = -MCTinvCameraPos;
		const glm::vec3 positionOffset = tracePositionStream[i] + MCTinvCameraPos;
		const glm::vec3 beginDirection = MCTcameraDirection;
		const glm::vec3 directionOffset = traceDirectionStream[i] - MCTcameraDirection;

		while (accumulatedFrameTime < samplingTime && !stopRequested.load()) {
			const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::high_resolution_clock::now() - loopEnd);
			const float interpolation = accumulatedFrameTime / samplingTime;
			MCTinvCameraPos = -(beginPosition + interpolation * positionOffset);
			MCTcameraDirection = glm::normalize(beginDirection + interpolation * directionOffset);
			accumulatedFrameTime += static_cast<float>(duration.count());
			loopEnd = std::chrono::high_resolution_clock::now();
			deltaTimeValues.push_back(deltaTime);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	if (stopRequested.load() || deltaTimeValues.empty()) {
		isTracking.store(false);
		freeCam = false;
		return;
	}

	const int lowOnePercentSize = std::max(1, static_cast<int>(deltaTimeValues.size() / 100));
	std::sort(deltaTimeValues.begin(), deltaTimeValues.end(), std::greater<float>());
	const float lowOnePercentFrameTime = std::accumulate(
		deltaTimeValues.begin(), deltaTimeValues.begin() + lowOnePercentSize, 0.0f) / lowOnePercentSize;
	const float averageFrameTime = std::accumulate(
		deltaTimeValues.begin(), deltaTimeValues.end(), 0.0f) / deltaTimeValues.size();
	const auto duration = std::chrono::duration_cast<std::chrono::seconds>(
		std::chrono::high_resolution_clock::now() - start);

	std::string benchmarkInfo = "----------Hardware Info----------\n";
	benchmarkInfo += getHardWareInfo() + "\n\n";
	benchmarkInfo += "----------Settings----------\n";
	benchmarkInfo += "Full Screen: " + std::string(FULL_SCREEN ? "On" : "Off") + "\n";
	benchmarkInfo += "Resolution: " + std::to_string(OUTER_WIDTH) + "x" + std::to_string(OUTER_HEIGHT) + "\n";
	benchmarkInfo += "Inner Resolution: " + std::to_string(INNER_WIDTH) + "x" + std::to_string(INNER_HEIGHT) + "\n";
	benchmarkInfo += "FOV: " + std::to_string(FOV) + "\n";
	benchmarkInfo += "RADIANCE_CACHE_RAD: " + std::to_string(RADIANCE_CACHE_RAD) + "\n";
	benchmarkInfo += "Infinity Diffuse: " + std::string(debugVal ? "On" : "Off") + "\n";
	benchmarkInfo += "RTGI SSP: " + std::to_string(SSP) + "\n";
	benchmarkInfo += "Secondary SSP: " + std::to_string(SSP_2) + "\n\n";
	benchmarkInfo += "----------Benchmark Result----------\n";
	benchmarkInfo += "Time: " + std::to_string(duration.count()) + "s\n";
	benchmarkInfo += "Avg FrameTime : " + std::to_string(averageFrameTime * 1000) + "ms\n";
	benchmarkInfo += "Avg Fps: " + std::to_string(1.0f / averageFrameTime) + "\n";
	benchmarkInfo += "Low1 % FrameTime : " + std::to_string(lowOnePercentFrameTime * 1000) + "ms\n";
	benchmarkInfo += "Low1% Fps: " + std::to_string(1.0f / lowOnePercentFrameTime) + "\n\n";

	std::ofstream outfile("res/benchmarkInfo.txt");
	outfile << benchmarkInfo;
	std::cout << benchmarkInfo << std::endl;
	isTracking.store(false);
	freeCam = false;
	displayID = 16;
}

MCameraTrack::~MCameraTrack()
{
	stop();
}

void MCameraTrack::endRecord()
{
	isRecording.store(false);
}

bool MCameraTrack::traceDecode(const std::string& path, std::string& error)
{
	traceStream.clear();
	tracePositionStream.clear();
	traceDirectionStream.clear();

	std::ifstream file(path);
	if (!file.is_open()) {
		error = "could not open camera track: " + path;
		return false;
	}

	try {
		std::string line;
		if (!std::getline(file, line)) {
			error = "camera track is empty: " + path;
			return false;
		}
		std::stringstream stream(line);
		std::string value;
		if (!std::getline(stream, value, ',')) {
			throw std::invalid_argument("missing sampling rate");
		}
		samplingHz = std::stof(value);
		if (!std::getline(stream, value, ',')) {
			throw std::invalid_argument("missing duration");
		}
		maxSecond = static_cast<int>(std::stof(value));
		while (std::getline(stream, value, ',')) {
			if (!value.empty()) {
				traceStream.push_back(std::stof(value));
			}
		}
	}
	catch (const std::exception& exception) {
		error = std::string("invalid camera track: ") + exception.what();
		return false;
	}

	if (samplingHz <= 0.0f || traceStream.empty() || traceStream.size() % 6 != 0) {
		error = "camera track must contain a positive sampling rate and complete position/direction samples";
		return false;
	}
	for (std::size_t i = 0; i < traceStream.size() / 6; ++i) {
		tracePositionStream.push_back(glm::vec3(
			traceStream[6 * i], traceStream[6 * i + 1], traceStream[6 * i + 2]));
		traceDirectionStream.push_back(glm::vec3(
			traceStream[6 * i + 3], traceStream[6 * i + 4], traceStream[6 * i + 5]));
	}
	return true;
}

bool MCameraTrack::beginExecute(std::string& error)
{
	if (tracePositionStream.empty() || samplingHz <= 0.0f) {
		error = "camera track has not been decoded";
		return false;
	}
	if (executionThread.joinable()) {
		error = "camera track is already running";
		return false;
	}
	stopRequested.store(false);
	isTracking.store(true);
	executionThread = std::thread(&MCameraTrack::traceExecuting, this);
	return true;
}

void MCameraTrack::beginRecord(float requestedSamplingHz, float requestedMaxSecond, std::string path)
{
	if (requestedSamplingHz <= 0.0f || requestedMaxSecond <= 0.0f) {
		throw std::invalid_argument("camera recording rate and duration must be positive");
	}
	if (recordingThread.joinable()) {
		recordingThread.join();
	}
	stopRequested.store(false);
	isRecording.store(true);
	traceStream.clear();
	recordingThread = std::thread(
		&MCameraTrack::traceSampling, this, requestedSamplingHz, requestedMaxSecond, std::move(path));
}

void MCameraTrack::stop()
{
	stopRequested.store(true);
	isRecording.store(false);
	if (recordingThread.joinable()) {
		recordingThread.join();
	}
	if (executionThread.joinable()) {
		executionThread.join();
	}
	isTracking.store(false);
}
