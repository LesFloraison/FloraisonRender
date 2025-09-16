#pragma once
#include "encapVk.h"
#include <iostream>
#include <chrono>
#include <thread>
class MCameraTrack
{
private:
	bool isRecording = false;
	void traceSampling(float m_samplingHz, float m_maxSecond, std::string m_path);
	void traceExcuting();
public:
	float samplingHz;
	int maxSecond;
	static bool isTracking;
	static glm::vec3 MCTcameraDirection;
	static glm::vec3 MCTinvCameraPos;
	std::vector<float> traceStream;
	std::vector<glm::vec3> tracePositionStream;
	std::vector<glm::vec3> traceDirectionStream;
	MCameraTrack();
	void beginRecord(float m_samplingHz, float m_maxSecond, std::string m_path);
	void endRecord();
	void traceDecode(std::string m_path);
	void beginExecute();
};

