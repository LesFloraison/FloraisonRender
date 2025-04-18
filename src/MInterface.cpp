#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include "MInterface.h"
#include "MPipeline.h"
#include <ft2build.h>
#include "JSON.h"
#include FT_FREETYPE_H

extern std::string consoleString;
int leftDown;
std::vector<VkImageView> MInterface::interfaceTextureArrayViews;
std::vector<VkImageView> MInterface::fontTextureArrayViews;
std::vector<int> MInterface::textDisableTable(64);
int MInterface::page = 0;

MInterface::MInterface(std::string m_path)
{
	path = m_path;
	aspectScale = OUTER_WIDTH / 1920.0f;
	initFont();
	loadInterface();
	loadStateFile();
}

void MInterface::loadInterface()
{
	std::ifstream file(path);
	std::string line;
	if (file.is_open()) {
		while (std::getline(file, line)) {
			if (line == "") {
				continue;
			}
			if (line[0] == '/') {
				continue;
			}

			JSON* json = new JSON(line);
			if (!json->exist("type")) {
				continue;
			}

			if (json->getValue<std::string>("type") == "texture") {
				int textureID = json->getValue<int>("id");
				std::string texturePath = json->getValue<std::string>("path");
				std::cout << texturePath << std::endl;
				VkImage* textureImage = new VkImage;
				VkDeviceMemory* textureImageMemory = new VkDeviceMemory;
				VkImageView* textureImageView = new VkImageView;
				createTexture(textureImageView, textureImage, textureImageMemory, (texturePath).c_str());
				interfaceTextureArrayViews.push_back(*textureImageView);
			}

			if (json->getValue<std::string>("type") == "tile") {
				int page = json->getValue<int>("page");
				float layer = 0.9 * (10 - json->getValue<int>("layer")) / 10.0f;
				glm::vec2 minVertex = toVec2(json->getVector<float>("minvertex"));
				glm::vec2 maxVertex = toVec2(json->getVector<float>("maxvertex"));
				int textureID = json->getValue<int>("texture");
				//int id = json->exist("id") ? json->getValue<int>("id") : -1;
				int id = -1;
				if (json->exist("id")) {
					id = json->getValue<int>("id");
				}

				float rad = json->getValue<float>("rad");
				std::string executeString = json->getValue<std::string>("execute");
				for (int i = 0; i < 6; i++) {
					if (i == 0) {
						interfaceVertexStream.push_back((maxVertex.x + page) * 2 - 1);
						interfaceVertexStream.push_back(maxVertex.y * 2 - 1);
						interfaceVertexStream.push_back(layer);
						interfaceVertexStream.push_back(1);
						interfaceVertexStream.push_back(0);
					}
					if (i == 1) {
						interfaceVertexStream.push_back((maxVertex.x + page) * 2 - 1);
						interfaceVertexStream.push_back(minVertex.y * 2 - 1);
						interfaceVertexStream.push_back(layer);
						interfaceVertexStream.push_back(1);
						interfaceVertexStream.push_back(1);
					}
					if (i == 2) {
						interfaceVertexStream.push_back((minVertex.x + page) * 2 - 1);
						interfaceVertexStream.push_back(maxVertex.y * 2 - 1);
						interfaceVertexStream.push_back(layer);
						interfaceVertexStream.push_back(0);
						interfaceVertexStream.push_back(0);
					}
					if (i == 3) {
						interfaceVertexStream.push_back((maxVertex.x + page) * 2 - 1);
						interfaceVertexStream.push_back(minVertex.y * 2 - 1);
						interfaceVertexStream.push_back(layer);
						interfaceVertexStream.push_back(1);
						interfaceVertexStream.push_back(1);
					}
					if (i == 4) {
						interfaceVertexStream.push_back((minVertex.x + page) * 2 - 1);
						interfaceVertexStream.push_back(minVertex.y * 2 - 1);
						interfaceVertexStream.push_back(layer);
						interfaceVertexStream.push_back(0);
						interfaceVertexStream.push_back(1);
					}
					if (i == 5) {
						interfaceVertexStream.push_back((minVertex.x + page) * 2 - 1);
						interfaceVertexStream.push_back((maxVertex.y) * 2 - 1);
						interfaceVertexStream.push_back(layer);
						interfaceVertexStream.push_back(0);
						interfaceVertexStream.push_back(0);
					}
					interfaceVertexStream.push_back(textureID);//texID
					interfaceVertexStream.push_back(0);
					interfaceVertexStream.push_back(0);
					interfaceVertexStream.push_back(rad);//style
					interfaceVertexStream.push_back(0);
					interfaceVertexStream.push_back(0);
				}
				std::cout << "tile id:" << id << std::endl;
				Tile tmpTile;
				tmpTile.page = page;
				tmpTile.minVertex = minVertex;
				tmpTile.maxVertex = maxVertex;
				tmpTile.id = id;
				while (executeString.find("|") != std::string::npos) {
					std::string task = executeString.substr(0, executeString.find("|"));
					executeString = executeString.substr(executeString.find("|") + 1);
					//std::cout << task << std::endl;
					//std::cout << executeString << std::endl;
					tmpTile.excuteString.push_back(task);
				}
				tmpTile.excuteString.push_back(executeString);
				//tmpTile.excuteString = executeString;
				std::cout << "group:" << tmpTile.excuteString.size() << std::endl;
				tileList.push_back(tmpTile);
			}


			if (json->getValue<std::string>("type") == "text") {
				int page = json->getValue<int>("page");
				float layer = 0.9 * (10 - json->getValue<int>("layer")) / 10.0f;
				glm::vec2 origin = toVec2(json->getVector<float>("origin"));
				glm::vec3 color = toVec3(json->getVector<float>("color"));
				float scale = json->getValue<float>("scale");
				scale *= aspectScale;
				std::string textString = json->getValue<std::string>("text");
				int flag = json->getValue<int>("flag");

				float x = origin.x * OUTER_WIDTH;
				float y = origin.y * OUTER_HEIGHT;

				for (int i = 0; i < textString.size(); i++) {
					char c = textString[i];
					//cout << c << endl;
					Character ch = CharactersTable[c];
					float xpos = x + ch.bearing.x * scale;
					float ypos = y + (ch.size.y - ch.bearing.y) * scale;
					//cout << xpos << "   " << ypos << endl;
					float w = ch.size.x * scale;
					float h = ch.size.y * scale;
					for (int j = 0; j < 6; j++) {
						if (j == 0) {
							textVertexStream.push_back(((xpos) / OUTER_WIDTH + page) * 2 - 1);
							textVertexStream.push_back(((ypos - h) / OUTER_HEIGHT) * 2 - 1);
							textVertexStream.push_back(layer);
							textVertexStream.push_back(0);
							textVertexStream.push_back(0);
						}
						if (j == 1) {
							textVertexStream.push_back(((xpos) / OUTER_WIDTH + page) * 2 - 1);
							textVertexStream.push_back(((ypos) / OUTER_HEIGHT) * 2 - 1);
							textVertexStream.push_back(layer);
							textVertexStream.push_back(0);
							textVertexStream.push_back(1);
						}
						if (j == 2) {
							textVertexStream.push_back(((xpos + w) / OUTER_WIDTH + page) * 2 - 1);
							textVertexStream.push_back(((ypos) / OUTER_HEIGHT) * 2 - 1);
							textVertexStream.push_back(layer);
							textVertexStream.push_back(1);
							textVertexStream.push_back(1);
						}
						if (j == 3) {
							textVertexStream.push_back(((xpos) / OUTER_WIDTH + page) * 2 - 1);
							textVertexStream.push_back(((ypos - h) / OUTER_HEIGHT) * 2 - 1);
							textVertexStream.push_back(layer);
							textVertexStream.push_back(0);
							textVertexStream.push_back(0);
						}
						if (j == 4) {
							textVertexStream.push_back(((xpos + w) / OUTER_WIDTH + page) * 2 - 1);
							textVertexStream.push_back(((ypos) / OUTER_HEIGHT) * 2 - 1);
							textVertexStream.push_back(layer);
							textVertexStream.push_back(1);
							textVertexStream.push_back(1);
						}
						if (j == 5) {
							textVertexStream.push_back(((xpos + w) / OUTER_WIDTH + page) * 2 - 1);
							textVertexStream.push_back(((ypos - h) / OUTER_HEIGHT) * 2 - 1);
							textVertexStream.push_back(layer);
							textVertexStream.push_back(1);
							textVertexStream.push_back(0);
						}
						textVertexStream.push_back(c);//texID
						textVertexStream.push_back(flag);//textFlag
						textVertexStream.push_back(0);
						textVertexStream.push_back(color.x);//color
						textVertexStream.push_back(color.y);
						textVertexStream.push_back(color.z);
					}
					x += (ch.advance >> 6) * scale;
				}

			}
			
		}
		file.close();
	}

	void* interfaceVerticesData = interfaceVertexStream.data();
	VkDeviceSize interfaceVerticesBufferSize = sizeof(float) * interfaceVertexStream.size();
	createVertexBuffer(&interfaceVertexBuffer, &interfaceVertexMemory, &interfaceVerticesData, interfaceVerticesBufferSize);

	void* textVerticesData = textVertexStream.data();
	VkDeviceSize textVerticesBufferSize = sizeof(float) * textVertexStream.size();
	createVertexBuffer(&textVertexBuffer, &textVertexMemory, &textVerticesData, textVerticesBufferSize);
}

void MInterface::drawInterface(VkCommandBuffer commandBuffer, MPipeline* pipeline, VkPipelineLayout pipelineLayout)
{
	uint64_t offsets = 0;
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &pipeline->descriptorSets, 0, nullptr);
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &interfaceVertexBuffer, &offsets);
	universalPushConst interfacePushConst;
	interfacePushConst.v4 = glm::vec4(OUTER_WIDTH, OUTER_HEIGHT, 10, 10);
	interfacePushConst.m4 = glm::mat4(1);
	vkCmdPushConstants(commandBuffer, pipeline->universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &interfacePushConst);
	vkCmdDraw(commandBuffer, 66, 1, 0, 0);
}

void MInterface::executionTrigger()
{
	if (displayID != 16) {
		return;
	}
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && leftDown == 0) {
		leftDown = 1;
		float xPos = lastX / OUTER_WIDTH;
		float yPos = lastY / OUTER_HEIGHT;
		for (Tile &tile : tileList) {
			if (xPos > tile.minVertex.x && yPos > tile.minVertex.y && xPos < tile.maxVertex.x && yPos < tile.maxVertex.y && page == tile.page) {
				executeSingle(tile.excuteString[tile.state]);
				tile.state++;
				tile.state %= tile.excuteString.size();
			}
		}
	}
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
		leftDown = 0;
	}
}

void MInterface::initFont()
{
	FT_Library ft;
	if (FT_Init_FreeType(&ft)) {
		std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
	}

	FT_Face face;
	if (FT_New_Face(ft, "res/fonts/arial.ttf", 0, &face)) {
		std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
	}

	FT_Set_Pixel_Sizes(face, 0, 48);

	for (unsigned char c = 0; c < 127; c++)
	{
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
		{
			std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
			continue;
		}

		Character character;
		character.size = glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
		character.bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
		character.advance = face->glyph->advance.x;
		CharactersTable.push_back(character);

		VkDeviceSize cImageSize = face->glyph->bitmap.width * face->glyph->bitmap.rows * 1;

		if (c == 32) {
			FT_Load_Char(face, '0', FT_LOAD_RENDER);
			cImageSize = face->glyph->bitmap.width * face->glyph->bitmap.rows * 1;
		}

		//cout << c << int(c)<<endl;
		//cout << face->glyph->bitmap.width << "   " << face->glyph->bitmap.rows << endl;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(cImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, cImageSize, 0, &data);
		memcpy(data, face->glyph->bitmap.buffer, static_cast<size_t>(cImageSize));
		vkUnmapMemory(device, stagingBufferMemory);

		VkImage* cImage = new VkImage;
		VkDeviceMemory* cImageMemory = new VkDeviceMemory;
		createImage(cImage, cImageMemory, face->glyph->bitmap.width, face->glyph->bitmap.rows, 1, VK_FORMAT_R8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		transitionImageLayout(*cImage, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyBufferToImage(stagingBuffer, *cImage, static_cast<uint32_t>(face->glyph->bitmap.width), static_cast<uint32_t>(face->glyph->bitmap.rows));
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
		generateMipmaps(*cImage, face->glyph->bitmap.width, face->glyph->bitmap.rows, 1);

		VkImageView* cImageView = new VkImageView;
		createImageView(cImageView, *cImage, 1, VK_FORMAT_R8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
		fontTextureArrayViews.push_back(*cImageView);
	}
	FT_Done_Face(face);
	FT_Done_FreeType(ft);
}

void MInterface::writeStateFile() {
	std::ofstream outfile("res/interface/state.txt");
	std::string stateContent = std::string();
	for (Tile& tile : tileList) {
		if (tile.id != -1) {
			stateContent += std::string("{\"tile_id\":") + std::to_string(tile.id) + std::string(",\"state\":") + std::to_string(tile.state) + std::string("}\n");
		}
	}
	outfile << stateContent;
	outfile.close();
}

void MInterface::loadStateFile() {
	std::ifstream file("res/interface/state.txt");
	std::string line;
	if (file.is_open()) {
		while (std::getline(file, line)) {
			if (line == "") {
				continue;
			}
			if (line[0] == '/') {
				continue;
			}
			JSON* json = new JSON(line);
			if (!json->exist("type")) {
				continue;
			}
			if (json->getValue<std::string>("type") == "state") {
				int id = json->getValue<int>("tile_id");
				int state = json->getValue<int>("state");
				for (Tile& tile : tileList) {
					if (id == tile.id) {
						executeSingle(tile.excuteString[(state) == 0 ? tile.excuteString.size() - 1 : state - 1]);
						tile.state = state;
					}
				}
			}
		}
	}
}