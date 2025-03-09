#pragma once
#define AL_LIBTYPE_STATIC
#define SAMPLE_RATE 44100
#include "encapVk.h"
#include <stdlib.h>
#include <string>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <vector>
#include <AL/al.h>
#include <AL/alc.h>
class MAudio
{
public:
	MAudio(std::string path);
	~MAudio();

	struct AudioInfo {
		ALuint source_id;
		ALuint buffer_id;
		glm::vec3 absPosition;
		glm::ivec3 axisFollowing;
		ALint isLooping;
	};
	ALCdevice* device;
	ALCcontext* context;
	std::vector<AudioInfo> audioInfos;

	void loadAudioSource(std::string audioPath, glm::vec3 m_absPosition, glm::ivec3 m_axisFollowing, ALint m_isLooping);
	void audioUpdate();
};

