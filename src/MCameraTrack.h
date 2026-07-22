#pragma once

#include "encapVk.h"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

class MCameraTrack
{
private:
	std::atomic_bool isRecording{ false };
	std::atomic_bool stopRequested{ false };
	std::thread recordingThread;
	std::thread executionThread;

	void traceSampling(float samplingHz, float maxSecond, std::string path);
	void traceExecuting();

public:
	float samplingHz = 0.0f;
	int maxSecond = 0;
	static std::atomic_bool isTracking;
	static glm::vec3 MCTcameraDirection;
	static glm::vec3 MCTinvCameraPos;
	std::vector<float> traceStream;
	std::vector<glm::vec3> tracePositionStream;
	std::vector<glm::vec3> traceDirectionStream;

	MCameraTrack() = default;
	~MCameraTrack();
	MCameraTrack(const MCameraTrack&) = delete;
	MCameraTrack& operator=(const MCameraTrack&) = delete;

	void beginRecord(float samplingHz, float maxSecond, std::string path);
	void endRecord();
	bool traceDecode(const std::string& path, std::string& error);
	bool beginExecute(std::string& error);
	void stop();
};
