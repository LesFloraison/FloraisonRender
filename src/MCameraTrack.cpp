#include "MCameraTrack.h"
#include <fstream>
#include <algorithm>
#include <numeric>
bool MCameraTrack::isTracking;
glm::vec3 MCameraTrack::MCTcameraDirection;
glm::vec3 MCameraTrack::MCTinvCameraPos;
void MCameraTrack::traceSampling(float m_samplingHz, float m_maxSecond, std::string m_path)
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

void MCameraTrack::traceExcuting()
{
	std::vector<float> deltaTimeVec;
	freeCam = true;
	auto start = std::chrono::high_resolution_clock::now();
	MCTinvCameraPos = -tracePositionStream[0];
	MCTcameraDirection = traceDirectionStream[0];
	float samplingTime = 1000 / samplingHz;
	for (int i = 0; i < tracePositionStream.size(); i++) {
		float frameTimeAccu = 0;
		auto loopEnd = std::chrono::high_resolution_clock::now();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		glm::vec3 beginPos = -MCTinvCameraPos;
		glm::vec3 stepOffset = tracePositionStream[i] + MCTinvCameraPos;

		glm::vec3 beginDirection = MCTcameraDirection;
		glm::vec3 directionOffset = traceDirectionStream[i] - MCTcameraDirection;
		while (frameTimeAccu < samplingTime) {
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - loopEnd);
			float C = frameTimeAccu / samplingTime;
			glm::vec3 newPosition = beginPos + C * stepOffset;
			MCTinvCameraPos = -newPosition;
	
			glm::vec3 newDirection = glm::normalize(beginDirection + C * directionOffset);
			MCTcameraDirection = newDirection;

			frameTimeAccu += duration.count();
			loopEnd = std::chrono::high_resolution_clock::now();
			deltaTimeVec.push_back(deltaTime);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
	int low1size = deltaTimeVec.size() / 100;
	std::sort(deltaTimeVec.begin(), deltaTimeVec.end(),std::greater<float>());
	float low1FrameTime = std::accumulate(deltaTimeVec.begin(), deltaTimeVec.begin() + low1size, 0.0) / low1size;
	float avgFrameTime = std::accumulate(deltaTimeVec.begin(), deltaTimeVec.end(), 0.0) / deltaTimeVec.size();
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);

	std::string benchmarkInfo = std::string("----------Hardware Info----------\n");
	benchmarkInfo += getHardWareInfo() + std::string("\n");
	benchmarkInfo += std::string("\n");

	benchmarkInfo += std::string("----------Settings----------\n");
	benchmarkInfo += std::string("Full Screen: ") + (FULL_SCREEN ? std::string("On") : std::string("Off")) + std::string("\n");
	benchmarkInfo += std::string("Resolution: ") + std::to_string(OUTER_WIDTH) + std::string("x") + std::to_string(OUTER_HEIGHT) + std::string("\n");
	benchmarkInfo += std::string("Inner Resolution: ") + std::to_string(INNER_WIDTH) + std::string("x") + std::to_string(INNER_HEIGHT) + std::string("\n");
	benchmarkInfo += std::string("FOV: ") + std::to_string(FOV) + std::string("\n");
	benchmarkInfo += std::string("RADIANCE_CACHE_RAD: ") + std::to_string(RADIANCE_CACHE_RAD) + std::string("\n");
	benchmarkInfo += std::string("Infinity Diffuse: ") + (debugVal ? std::string("On") : std::string("Off")) + std::string("\n");
	benchmarkInfo += std::string("RTGI SSP: ") + std::to_string(SSP) + std::string("\n");
	benchmarkInfo += std::string("Secondary SSP: ") + std::to_string(SSP_2) + std::string("\n");
	benchmarkInfo += std::string("\n");

	benchmarkInfo += std::string("----------Benchmark Result----------\n");
	benchmarkInfo += std::string("Time: ") + std::to_string(duration.count()) + std::string("s\n");
	benchmarkInfo += std::string("Avg FrameTime : ") + std::to_string(avgFrameTime * 1000) + std::string("ms\n");
	benchmarkInfo += std::string("Avg Fps: ") + std::to_string(1.0 / avgFrameTime) + std::string("\n");
	benchmarkInfo += std::string("Low1 % Frame Time : ") + std::to_string(low1FrameTime * 1000) + std::string("ms\n");
	benchmarkInfo += std::string("Low1% Fps: ") + std::to_string(1.0 / low1FrameTime) + std::string("\n");
	benchmarkInfo += std::string("\n");

	std::ofstream outfile("res/benchmarkInfo.txt");
	outfile << benchmarkInfo;
	outfile.close();
	std::cout << benchmarkInfo << std::endl;
	isTracking = false;
	freeCam = false;
	displayID = 16;
}

MCameraTrack::MCameraTrack()
{
}

void MCameraTrack::endRecord()
{
	isRecording = false;
}

void MCameraTrack::traceDecode(std::string m_path)
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

void MCameraTrack::beginExecute()
{
	isTracking = true;
	std::thread traceExcutingThread(&MCameraTrack::traceExcuting, this);
	traceExcutingThread.detach();
}

void MCameraTrack::beginRecord(float m_samplingHz, float m_maxSecond, std::string m_path)
{
	isRecording = true;
	traceStream.empty();
	std::thread traceSamplingThread(&MCameraTrack::traceSampling, this, m_samplingHz, m_maxSecond, m_path);
	traceSamplingThread.detach();
}
