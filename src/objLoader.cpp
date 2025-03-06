#pragma once
#include "objLoader.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <stb_image.h>
#define STB_IMAGE_IMPLEMENTATION
#include <GLFW/glfw3.h>
#include <string>
#include <Volk/volk.h>
#include "encapVk.h"
#include "MRenderCore.h"
#define LineLength 256
using namespace glm;
using namespace std;

VkBuffer objLoader::objReferenceBuffer;
VkDeviceMemory objLoader::objReferenceBufferMemory;
vector<float> objLoader::objReferenceStream = { 0 };
int objLoader::objReferenceCount = 0;

objLoader::objLoader(const char* path)
{
	directory = string(path);
	directory = directory.substr(0, directory.find_last_of('/'));
	cout << directory << endl;
	char buffer[LineLength];
	char temp[10] = { 0 };
	vec3 tempv3;
	vec2 tempv2;
	fstream out;
	stbi_set_flip_vertically_on_load(true);

	int albeCount = 0;
	int armCount = 0;
	int normalCount = 0;
	out.open(string(path).substr(0, string(path).find_last_of('o')) + "mtl", ios::in);
	while (!out.eof())
	{
		int texCount = albeCount + armCount + normalCount;
		out.getline(buffer, LineLength, '\n');
		if (buffer[0] == 'n' && buffer[1] == 'e' && buffer[2] == 'w') {
			Material tempMat;
			string matName(64,0);
			for (int i = 7, j = 0; 1; i++, j++) {
				matName[j] = buffer[i];
				if (buffer[i] == '\0') {
					//TODO
					matName += path;
					break;
				}
			}
			//cout << matName << endl;
			tempMat.Name = matName;
			tempMat.albedoView = NULL;
			tempMat.armView = NULL;
			tempMat.normalView = NULL;
			MRenderCore::materialArray.push_back(tempMat);
		}
		if (buffer[4] == 'K' && buffer[5] == 'd' && MRenderCore::materialArray[MRenderCore::materialArray.size() - 1].albedoView == NULL) {
			albeCount++;
			string textureName(64,0);
			for (int i = 7, j = 0; 1; i++, j++) {
				textureName[j] = buffer[i];
				if (buffer[i] == '\0') {
					break;
				}
			}
			VkImage* albedoImage = new VkImage;
			VkDeviceMemory* albedoImageMemory = new VkDeviceMemory;
			VkImageView* albedoImageView = new VkImageView;
			cout << (directory + string("/") + textureName).c_str() << endl;
			createTexture(albedoImageView,albedoImage, albedoImageMemory, (directory + string("/") + textureName).c_str());
			MRenderCore::materialArray[MRenderCore::materialArray.size() - 1].albedoMemory = albedoImageMemory;
			MRenderCore::materialArray[MRenderCore::materialArray.size() - 1].albedoImage = albedoImage;
			MRenderCore::materialArray[MRenderCore::materialArray.size() - 1].albedoView = albedoImageView;

			MRenderCore::textureArrayViews.push_back(*albedoImageView);

			//TODO
		}
		
		if (buffer[4] == 'B' && buffer[5] == 'u' && MRenderCore::materialArray[MRenderCore::materialArray.size() - 1].normalView == NULL) {
			normalCount++;
			string textureName(64,0);
			for (int i = 9, j = 0; 1; i++, j++) {
				textureName[j] = buffer[i];
				if (buffer[i] == '\0') {
					break;
				}
			}
			VkImage* normalImage = new VkImage;
			VkDeviceMemory* normalImageMemory = new VkDeviceMemory;
			VkImageView* normalImageView = new VkImageView;
			cout << (directory + string("/") + textureName).c_str() << endl;
			createTexture(normalImageView, normalImage, normalImageMemory, (directory + string("/") + textureName).c_str());
			MRenderCore::materialArray[MRenderCore::materialArray.size() - 1].normalMemory = normalImageMemory;
			MRenderCore::materialArray[MRenderCore::materialArray.size() - 1].normalImage = normalImage;
			MRenderCore::materialArray[MRenderCore::materialArray.size() - 1].normalView = normalImageView;

			MRenderCore::textureArrayViews.push_back(*normalImageView);

			//TODO
		}
	}
	cout << "albeTex:" << albeCount << endl;
	cout << "armTex:" << armCount << endl;
	cout << "normalTex:" << normalCount << endl;
	out.close();

	bool inf = false;
	//meshSet.push_back(0);
	out.open(path, ios::in);
	int currAlbedoIndex = -1;
	int currNormalIndex = -1;
	int currArmIndex = -1;
	while (!out.eof())
	{
		//linesCount++;
		//cout << linesCount << endl;
		out.getline(buffer, LineLength, '\n');//getline(char *,int,char) 表示该行字符达到256个或遇到换行就结束
		if (buffer[0] == 's') {
			continue;
		}
		if (inf == true && buffer[0] != 'f') {
			//meshSet.push_back(vertexStream.size());
			inf = false;
		}
		if (buffer[0] == 'u' && buffer[1] == 's') {
			string matName(64,0);
			for (int i = 7, j = 0; 1; i++, j++) {
				matName[j] = buffer[i];
				if (buffer[i] == '\0') {
					matName += path;
					break;
				}
			}
			//cout << matName << endl;
			//cout << linesCount << endl;
			//materialStream.push_back(matName);
			int validIndex = -1;
			for (int i = 0; i < MRenderCore::materialArray.size(); i++) {
				if (matName == MRenderCore::materialArray[i].Name) {
					validIndex = (MRenderCore::materialArray[i].albedoView == NULL) ? validIndex : validIndex + 1;
					currAlbedoIndex = (MRenderCore::materialArray[i].albedoView == NULL) ? -1 : validIndex;
					validIndex = (MRenderCore::materialArray[i].normalView == NULL) ? validIndex : validIndex + 1;
					currNormalIndex = (MRenderCore::materialArray[i].normalView == NULL) ? -1 : validIndex;
					validIndex = (MRenderCore::materialArray[i].armView == NULL) ? validIndex : validIndex + 1;
					currArmIndex = (MRenderCore::materialArray[i].armView == NULL) ? -1 : validIndex;
					break;
				}
				validIndex = (MRenderCore::materialArray[i].albedoView == NULL) ? validIndex : validIndex + 1;
				validIndex = (MRenderCore::materialArray[i].normalView == NULL) ? validIndex : validIndex + 1;
				validIndex = (MRenderCore::materialArray[i].armView == NULL) ? validIndex : validIndex + 1;
			}

		}

		if (buffer[0] == 'v' && buffer[1] != 't' && buffer[1] != 'n') {
			int vp = 0;
			for (int i = 0; i < LineLength; i++) {
				if (buffer[i] == ' ') {
					int j = 0;
					for (i = i + 1; 1; i++) {

						if (buffer[i] == ' ' || buffer[i] == '\0'||j>6) {    //repair
							i--;
							temp[j] = '\0';
							break;
						}
						temp[j] = buffer[i];
						j++;

					}
					(vp == 0 ? tempv3.x : (vp == 1 ? tempv3.y : tempv3.z)) = atof(temp);
					temp[0] = '\0';
					//cout << atof(temp) << endl;
					vp++;
				}
				if (buffer[i] == '\0') {
					break;
				}
			}
			vPos.push_back(tempv3);
			aabbMin = glm::min(aabbMin, tempv3);
			aabbMax = glm::max(aabbMax, tempv3);
		}

		if (buffer[0] == 'v' && buffer[1] == 'n') {
			//printf("vn\n");
			int vp = 0;
			for (int i = 0; i < LineLength; i++) {
				if (buffer[i] == ' ') {
					int j = 0;
					for (i = i + 1; 1; i++) {

						if (buffer[i] == ' ' || buffer[i] == '\0' || j > 6) {
							i--;
							temp[j] = '\0';
							break;
						}
						temp[j] = buffer[i];
						j++;

					}
					//cout << atof(temp) << endl;
					(vp == 0 ? tempv3.x : (vp == 1 ? tempv3.y : tempv3.z)) = atof(temp);
					temp[0] = '\0';
					vp++;
				}
				if (buffer[i] == '\0') {
					break;
				}
			}
			vNormal.push_back(tempv3);
		}

		if (buffer[0] == 'v' && buffer[1] == 't') {
			//printf("vt\n");
			int vp = 0;
			for (int i = 0; i < LineLength; i++) {
				if (buffer[i] == ' ') {
					int j = 0;
					for (i = i + 1; 1; i++) {

						if (buffer[i] == ' ' || buffer[i] == '\0' || j > 6) {
							i--;
							temp[j] = '\0';
							break;
						}
						temp[j] = buffer[i];
						j++;

					}
					//cout << atof(temp) << endl;
					(vp == 0 ? tempv2.x : tempv2.y) = atof(temp);
					vp++;
				}
				if (buffer[i] == '\0') {
					break;
				}
			}
			texCoord.push_back(tempv2);
		}

		if (buffer[0] == 'f') {
			//printf("f\n");
			inf = true;
			int vp = 0;
			for (int i = 0; i < LineLength; i++) {
				if (buffer[i] == ' ') {
					int j = 0;
					char tmpc[8];
					for (i = i + 1; 1; i++) {

						if (buffer[i] == ' ' || buffer[i] == '\0') {
							i--;
							tmpc[j] = '\0';
							vertexStream.push_back(vNormal[atoi(tmpc) - 1].x);
							vertexStream.push_back(vNormal[atoi(tmpc) - 1].y);
							vertexStream.push_back(vNormal[atoi(tmpc) - 1].z);
							vp = -1;
							break;
						}
						else if (buffer[i] == '/') {
							tmpc[j] = '\0';
							if (vp == 0) {
								vertexStream.push_back(vPos[atoi(tmpc) - 1].x);
								vertexStream.push_back(vPos[atoi(tmpc) - 1].y);
								vertexStream.push_back(vPos[atoi(tmpc) - 1].z);

								vertexPosStream.push_back(vPos[atoi(tmpc) - 1].x);
								vertexPosStream.push_back(vPos[atoi(tmpc) - 1].y);
								vertexPosStream.push_back(vPos[atoi(tmpc) - 1].z);
							}
							else if (vp == 1) {
								vertexStream.push_back(texCoord[atoi(tmpc) - 1].x);
								vertexStream.push_back(texCoord[atoi(tmpc) - 1].y);
								//TODO
								vertexStream.push_back(currAlbedoIndex);
								vertexStream.push_back(currNormalIndex);
								vertexStream.push_back(currArmIndex);
								//cout << "("<<currAlbedoIndex << "," << currNormalIndex << "," << currArmIndex<<")";
								//cout << "(" << texCoord[atoi(tmpc) - 1].x << "," << texCoord[atoi(tmpc) - 1].y<< ")";
							}
							vp++;
							j = 0;
						}
						else {
							tmpc[j] = buffer[i];
							j++;
						}

					}
					vp++;
					//cout << tmpc << endl;
				}
				if (buffer[i] == '\0') {
					break;
				}
			}
		}
		//cout << buffer << endl;
	}
	out.close();

	//objReferenceIndex = objReferenceCount;
	beginAt = objReferenceStream.size();
	objReferenceStream.insert(objReferenceStream.end(), vertexStream.begin(), vertexStream.end());
	void* objReferenceData = objReferenceStream.data();
	VkDeviceSize objReferenceBufferSize = sizeof(float) * objReferenceStream.size();
	updateStorageBuffer(&objReferenceBuffer, &objReferenceData, objReferenceBufferSize);

	void* vertexData = vertexStream.data();
	VkDeviceSize vertexBufferSize = sizeof(float) * vertexStream.size();
	createVertexBuffer(&vertexBuffer, &vertexBufferMemory, &vertexData, vertexBufferSize);

	void* vertexPosData = vertexPosStream.data();
	VkDeviceSize vertexPosBufferSize = sizeof(float) * vertexPosStream.size();
	createVertexBuffer(&vertexPosBuffer, &vertexPosBufferMemory, &vertexPosData, vertexPosBufferSize);

	createBLAS();
}

void objLoader::createBLAS()
{
	VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
	triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	triangles.vertexData.deviceAddress = getBufferDeviceAddress(vertexPosBuffer);
	triangles.vertexStride = 3 * sizeof(float);
	triangles.indexType = VK_INDEX_TYPE_NONE_KHR;
	triangles.indexData.deviceAddress = 0;
	triangles.maxVertex = static_cast<uint32_t>(vertexPosStream.size() / 3);
	triangles.transformData.deviceAddress = 0;
	triangles.transformData.hostAddress = nullptr;

	VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
	accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	accelerationStructureGeometry.geometry.triangles = triangles;


	VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
	accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	accelerationBuildGeometryInfo.geometryCount = 1;
	accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
	accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	accelerationStructureBuildSizesInfo.pNext = nullptr;

	uint32_t numTriangles = vertexPosStream.size() / 9;
	vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &accelerationBuildGeometryInfo, &numTriangles, &accelerationStructureBuildSizesInfo);

	createBufferWithAddress(accelerationStructureBuildSizesInfo.accelerationStructureSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, BLASBuffer, BLASMemory);

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelerationStructureCreateInfo.buffer = BLASBuffer;
	accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
	accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	vkCreateAccelerationStructureKHR(device, &accelerationStructureCreateInfo, nullptr, &BLAS);

	VkBuffer scratchBuffer;
	VkDeviceMemory scratchBufferMemory;
	createBufferWithAddress(accelerationStructureBuildSizesInfo.buildScratchSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, scratchBuffer, scratchBufferMemory);

	accelerationBuildGeometryInfo.dstAccelerationStructure = BLAS;
	accelerationBuildGeometryInfo.scratchData.deviceAddress = getBufferDeviceAddress(scratchBuffer);

	VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
	accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
	accelerationStructureBuildRangeInfo.primitiveOffset = 0;
	accelerationStructureBuildRangeInfo.firstVertex = 0;
	accelerationStructureBuildRangeInfo.transformOffset = 0;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };
	VkCommandBuffer asCommandBuffer = beginSingleTimeCommands();
	vkCmdBuildAccelerationStructuresKHR(asCommandBuffer, 1, &accelerationBuildGeometryInfo, accelerationBuildStructureRangeInfos.data());
	endSingleTimeCommands(asCommandBuffer);

	MRenderCore::bufferPool.push_back(&BLASBuffer);
	MRenderCore::bufferMemoryPool.push_back(&BLASMemory);
	MRenderCore::asPool.push_back(&BLAS);

	vkDestroyBuffer(device, scratchBuffer, nullptr);
	vkFreeMemory(device, scratchBufferMemory, nullptr);
}



objLoader::~objLoader()
{
}
