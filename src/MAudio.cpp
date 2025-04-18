#include "MAudio.h"
#include"JSON.h"

MAudio::MAudio(std::string path) {
	device = alcOpenDevice(NULL);
	if (device) {
		context = alcCreateContext(device, NULL);
		alcMakeContextCurrent(context);
	}
	else {
		std::cerr << "device open failed" << std::endl;
	}

	std::ifstream file(path);
	std::string line;
	if (file.is_open()) {
		while (std::getline(file, line)) {
			if (line[0] == '/') {
				continue;
			}
			JSON* json = new JSON(line);
			if (!json->exist("type")) {
				continue;
			}

			if (json->getValue<std::string>("type") == "audio") {
				std::string audioPath = json->getValue<std::string>("path");
				glm::vec3 audioPos = toVec3(json->getVector<float>("audiopos"));
				glm::vec3 axisFollowing = toVec3(json->getVector<float>("axisfollowing"));
				int looping = json->getValue<int>("looping");
				loadAudioSource(audioPath, audioPos, axisFollowing, looping);
			}
		}
		file.close();
	}


	for (AudioInfo info : audioInfos) {
		alSourcePlay(info.source_id);
	}
}

MAudio::~MAudio() {
	for (AudioInfo info : audioInfos) {
		alDeleteSources(1, &info.source_id);
		alDeleteBuffers(1, &info.buffer_id);
	}
	alcMakeContextCurrent(NULL);
	alcDestroyContext(context);
	alcCloseDevice(device);
}

void MAudio::loadAudioSource(std::string audioPath, glm::vec3 m_absPosition, glm::ivec3 m_axisFollowing, ALint m_isLooping) {
	std::ifstream audioFile(audioPath, std::ios::binary | std::ios::in);
	audioFile.seekg(100, std::ios::beg);
	std::ifstream file(audioPath, std::ios::binary | std::ios::ate);
	std::streampos endPos = file.tellg();
	int audioSize = (int(endPos) - 100) / 4 * 4;
	file.close();
	unsigned char* audioBuffer = new unsigned char[audioSize];
	std::cout << audioSize << std::endl;
	audioFile.read((char*)audioBuffer, audioSize);
	audioFile.close();

	ALuint buffer, source;
	alGenBuffers(1, &buffer);
	alBufferData(buffer, AL_FORMAT_MONO16, audioBuffer, audioSize, SAMPLE_RATE);
	alGenSources(1, &source);
	alSourcei(source, AL_BUFFER, buffer);
	alSourcei(source, AL_LOOPING, m_isLooping);

	AudioInfo info = { source, buffer, m_absPosition, m_axisFollowing, m_isLooping };
	audioInfos.push_back(info);

	delete[] audioBuffer;
}

void MAudio::audioUpdate() {
	for (AudioInfo info : audioInfos) {
		ALfloat sourcePos[] = { 
			info.absPosition.x * (1-info.axisFollowing.x) + -invCameraPos.x * info.axisFollowing.x,
			info.absPosition.y* (1 - info.axisFollowing.y) + -invCameraPos.y * info.axisFollowing.y,
			info.absPosition.z* (1 - info.axisFollowing.z) + -invCameraPos.z * info.axisFollowing.z };
		alSourcefv(info.source_id, AL_POSITION, sourcePos);
	}

	ALfloat listenerPos[] = { -invCameraPos.x,-invCameraPos.y,-invCameraPos.z };
	alListenerfv(AL_POSITION, listenerPos);
}
