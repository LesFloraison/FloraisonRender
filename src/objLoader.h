#pragma once
#include <vector>
#include <iostream>
#include <fstream>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <GLFW/glfw3.h>
#include <string>
#include <Volk/volk.h>
using namespace std;
class objLoader
{

public:
	objLoader(const char* path);
	~objLoader();
	struct Material
	{
		string Name;
		VkImageView* albedoView;
		VkImageView* armView;
		VkImageView* normalView;
		VkImage* albedoImage;
		VkImage* armImage;
		VkImage* normalImage;
		VkDeviceMemory* albedoMemory;
		VkDeviceMemory* armMemory;
		VkDeviceMemory* normalMemory;
	};
	vector<float> vertexStream;
	vector<float> vertexPosStream;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer vertexPosBuffer;
	VkDeviceMemory vertexPosBufferMemory;
	VkAccelerationStructureKHR BLAS;
	VkBuffer BLASBuffer;
	VkDeviceMemory BLASMemory;

	static VkBuffer objReferenceBuffer;
	static VkDeviceMemory objReferenceBufferMemory;
	static vector<float>objReferenceStream;
	int beginAt;
	static int objReferenceCount;
	glm::vec3 aabbMin = glm::vec3(9999);
	glm::vec3 aabbMax = glm::vec3(-9999);
	void createBLAS();
protected:
	string directory;
	vector<glm::vec3> vPos;
	vector<glm::vec3> vNormal;
	vector<glm::vec2> texCoord;
};

