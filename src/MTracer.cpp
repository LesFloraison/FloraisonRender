#include "MTracer.h"
#include <fstream>
bool MTracer::isTracerActivating;
glm::vec3 MTracer::direction;
void MTracer::traceSampling(float m_samplingHz, float m_maxSecond, std::string m_path)
{
	glm::vec3 curPosition = -invCameraPos;
	while (curPosition == -invCameraPos) {
	}
	auto start = std::chrono::high_resolution_clock::now();
	while (isRecording) {
		traceStream.push_back(-invCameraPos.x);
		traceStream.push_back(-invCameraPos.y);
		traceStream.push_back(-invCameraPos.z);
		traceStream.push_back(cameraDirection.x);
		traceStream.push_back(cameraDirection.y);
		traceStream.push_back(cameraDirection.z);
		std::this_thread::sleep_for(std::chrono::milliseconds(int(1000 / m_samplingHz)));
		std::cout << "recording" << std::endl;
		if (traceStream.size() > m_maxSecond * m_samplingHz * 6) {
			isRecording = false;
		}
	}
	std::ofstream outfile(m_path);
	std::string recordingContent = std::string(std::to_string(m_samplingHz) + "," + std::to_string(m_maxSecond)) + ",";
	for (int i = 0; i < traceStream.size(); i++) {
		recordingContent += std::to_string(traceStream[i]);
		recordingContent += ',';
	}
	outfile << recordingContent;
	outfile.close();
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
	std::cout << "recordingTime: " << duration.count() << "s" << std::endl;
}

void MTracer::traceExcuting()
{
	auto start = std::chrono::high_resolution_clock::now();
	invCameraPos = -tracePositionStream[0];
	cameraDirection = traceDirectionStream[0];
	float samplingTime = 1000 / samplingHz;
	for (int i = 0; i < tracePositionStream.size(); i++) {
		float frameTimeAccu = 0;
		auto loopEnd = std::chrono::high_resolution_clock::now();
		while (frameTimeAccu < samplingTime) {
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - loopEnd);
			float C = duration.count() / (samplingTime - frameTimeAccu);
			glm::vec3 stepOffset = tracePositionStream[i] + invCameraPos;
			glm::vec3 newPosition = -invCameraPos + C * stepOffset;
			invCameraPos = -newPosition;

			glm::vec3 directionOffset = traceDirectionStream[i] - cameraDirection;
			glm::vec3 newDirection = glm::normalize(cameraDirection + C * directionOffset);
			direction = newDirection;

			frameTimeAccu += duration.count();
			loopEnd = std::chrono::high_resolution_clock::now();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
	std::cout << "Time: " << duration.count() << "s" << std::endl;
	isTracerActivating = false;
}

MTracer::MTracer()
{
}

void MTracer::endRecord()
{
	isRecording = false;
}

void MTracer::traceDecode(std::string m_path)
{
	traceStream.empty();
	tracePositionStream.empty();
	traceDirectionStream.empty();
	std::ifstream file(m_path);
	if (file.is_open()) {
		std::string line;
		std::getline(file, line);
		samplingHz = std::stof(line.substr(0, line.find(',')));
		line = line.substr(line.find(',') + 1, line.size() - line.find(','));
		maxSecond = std::stof(line.substr(0, line.find(',')));
		line = line.substr(line.find(',') + 1, line.size() - line.find(','));
		while (line.find(',') != line.length() - 1) {
			traceStream.push_back(std::stof(line.substr(0, line.find(','))));
			line = line.substr(line.find(',') + 1, line.size() - line.find(','));
		}
	}
	file.close();

	for (int i = 0; i < traceStream.size() / 6; i++) {
		tracePositionStream.push_back(glm::vec3(traceStream[6 * i + 0], traceStream[6 * i + 1], traceStream[6 * i + 2]));
		traceDirectionStream.push_back(glm::vec3(traceStream[6 * i + 3], traceStream[6 * i + 4], traceStream[6 * i + 5]));
	}
}

void MTracer::beginExecute()
{
	isTracerActivating = true;
	std::thread traceExcutingThread(&MTracer::traceExcuting, this);
	traceExcutingThread.detach();
}

void MTracer::beginRecord(float m_samplingHz, float m_maxSecond, std::string m_path)
{
	isRecording = true;
	traceStream.empty();
	std::thread traceSamplingThread(&MTracer::traceSampling, this, m_samplingHz, m_maxSecond, m_path);
	traceSamplingThread.detach();
}
