#pragma once
#include"encapVk.h"
#include <iostream>
#include <fstream>
class MInterface
{
public:
	static int page;
	static vector<VkImageView> interfaceTextureArrayViews;
	static vector<VkImageView> fontTextureArrayViews;
	static std::vector<int> textDisableTable;
	struct Tile {
		int page;
		int state = 0;
		int id = -1;
		glm::vec2 minVertex;
		glm::vec2 maxVertex;
		vector<string> excuteString;
	};
	string path;
	std::vector<float> interfaceVertexStream;
	std::vector<float> textVertexStream;
	std::vector<Tile> tileList;
	VkBuffer interfaceVertexBuffer;
	VkDeviceMemory interfaceVertexMemory;
	VkBuffer textVertexBuffer;
	VkDeviceMemory textVertexMemory;
	MInterface(string path);
	void drawInterface(VkCommandBuffer commandBuffer, MPipeline* pipeline, VkPipelineLayout pipelineLayout);
	void executionTrigger();
	void writeStateFile();
	void loadStateFile();

	struct Character {
		glm::ivec2 size;
		glm::ivec2 bearing;
		int advance;
	};
	std::vector<Character> CharactersTable;
	float aspectScale = 1;

private:
	void loadInterface();
	void initFont();
};

