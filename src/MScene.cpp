#include "MScene.h"
#include "MRenderCore.h"

glm::vec2 jitterBias[4] = {
	glm::vec2(0.5) * glm::vec2(1.0f / INNER_WIDTH, 1.0 / INNER_HEIGHT) * glm::vec2(1, 1),
	glm::vec2(0.5) * glm::vec2(1.0f / INNER_WIDTH, 1.0 / INNER_HEIGHT) * glm::vec2(-1, -1),
	glm::vec2(0.5) * glm::vec2(1.0f / INNER_WIDTH, 1.0 / INNER_HEIGHT) * glm::vec2(1, -1),
	glm::vec2(0.5) * glm::vec2(1.0f / INNER_WIDTH, 1.0 / INNER_HEIGHT) * glm::vec2(-1, 1)
};

MScene::MScene(string path)
{
	lightInfos.push_back(glm::vec3(-1,-1,-1));
	lightInfos.push_back(glm::vec3(0));
	waterLayer = new objLoader("res/model/waterLayer/waterLayer.obj");

	std::ifstream file(path);
	std::string line;
	if (file.is_open()) {
		while (std::getline(file, line)) {
			if (line[0] == '/') {
				continue;
			}
			if (line[2] == 'p') {
				string objPath = line.substr(9, line.find(',') - 10);
				objLoader* newOBJ = new objLoader(objPath.c_str());
				int objKey = stoi(line.substr(line.find(',') + 9, line.length() - line.find(',') - 10));
				objMap.insert(std::make_pair(objKey, newOBJ));
			}
			if (line[2] == 'd') {
				vector<float> content;
				string subLine = line;
				while (subLine.find(',') != string::npos) {
					int sub1 = subLine.find(':') == (string::npos) ? 9999 : (subLine[subLine.find(':') + 1] == '[' ? subLine.find(':') + 2 : subLine.find(':') + 1);
					int sub2 = subLine[subLine.find(',') + 1] == '"' ? 9999 : subLine.find(',') + 1;
					subLine = subLine.substr(min(sub1, sub2));
					content.push_back(stof(subLine.substr(0, min(min(subLine.find(','), subLine.find(']')), subLine.find('}')))));
				}
				glm::vec3 invDefaultPos = -glm::vec3(content[0], content[1], content[2]);
				invCameraPos = invDefaultPos;
			}
			if (line[2] == 'o') {
				vector<float> content;
				string subLine = line;
				while (subLine.find(',') != string::npos) {
					int sub1 = subLine.find(':') == (string::npos) ? 9999 : (subLine[subLine.find(':') + 1] == '[' ? subLine.find(':') + 2 : subLine.find(':') + 1);
					int sub2 = subLine[subLine.find(',') + 1] == '"' ? 9999 : subLine.find(',') + 1;
					subLine = subLine.substr(min(sub1, sub2));
					content.push_back(stof(subLine.substr(0, min(min(subLine.find(','), subLine.find(']')), subLine.find('}')))));
				}
				int index = content[0];
				float roughness = content[10];
				MObject::Transform objTrans;
				objTrans.position = glm::vec3(content[1], content[2], content[3]);
				objTrans.rotate = glm::vec3(content[4], content[5], content[6]);
				objTrans.scale = glm::vec3(content[7], content[8], content[9]);
				addObject(objMap[index], objTrans, roughness);
			}
			if (line[2] == 'w') {
				vector<float> content;
				string subLine = line;
				while (subLine.find(',') != string::npos) {
					int sub1 = subLine.find(':') == (string::npos) ? 9999 : (subLine[subLine.find(':') + 1] == '[' ? subLine.find(':') + 2 : subLine.find(':') + 1);
					int sub2 = subLine[subLine.find(',') + 1] == '"' ? 9999 : subLine.find(',') + 1;
					subLine = subLine.substr(min(sub1, sub2));
					content.push_back(stof(subLine.substr(0, min(min(subLine.find(','), subLine.find(']')), subLine.find('}')))));
				}
				waterLayerInfo info;
				info.position = glm::vec3(content[0], content[1], content[2]);
				info.scale = glm::vec2(content[3], content[4]);
				info.warpScale_1 = content[5];
				info.warpScale_2 = content[6];
				info.flow_1 = glm::vec2(content[7], content[8]);
				info.flow_2 = glm::vec2(content[9], content[10]);
				waterLayerInfos.push_back(info);
			}
			if (line[2] == 's') {
				string skyboxPath = line.substr(11, line.find('}') - 12);
				cout << skyboxPath << endl;
				createSkybox(skyboxPath);
			}
			if (line[7] == 'v') {
				vector<float> content;
				string subLine = line;
				while (subLine.find(',') != string::npos) {
					int sub1 = subLine.find(':') == (string::npos) ? 9999 : (subLine[subLine.find(':') + 1] == '[' ? subLine.find(':') + 2 : subLine.find(':') + 1);
					int sub2 = subLine[subLine.find(',') + 1] == '"' ? 9999 : subLine.find(',') + 1;
					subLine = subLine.substr(min(sub1, sub2));
					content.push_back(stof(subLine.substr(0, min(min(subLine.find(','), subLine.find(']')), subLine.find('}')))));
				}
				glm::vec3 lightVec = glm::vec3(content[0], content[1], content[2]);
				glm::vec3 lightColor = glm::vec3(content[3], content[4], content[5]);
				setDirectLight(lightVec, lightColor);
			}
			if (line[7] == 'p') {
				vector<float> content;
				string subLine = line;
				while (subLine.find(',') != string::npos) {
					int sub1 = subLine.find(':') == (string::npos) ? 9999 : (subLine[subLine.find(':') + 1] == '[' ? subLine.find(':') + 2 : subLine.find(':') + 1);
					int sub2 = subLine[subLine.find(',') + 1] == '"' ? 9999 : subLine.find(',') + 1;
					subLine = subLine.substr(min(sub1, sub2));
					content.push_back(stof(subLine.substr(0, min(min(subLine.find(','), subLine.find(']')), subLine.find('}')))));
				}
				glm::vec3 lightPosition = glm::vec3(content[0], content[1], content[2]);
				glm::vec3 lightScale = glm::vec3(content[3], content[4], content[5]);
				glm::vec3 lightColor = glm::vec3(content[6], content[7], content[8]);
				float lightMaxRad = content[9];
				cout << lightMaxRad << endl;
				addPointLight(lightPosition, lightScale, lightColor, lightMaxRad);
			}
		}
		file.close();
	}


	createSceneVertexBuffer();
	createInstanceStorageBuffer();
	createTLAS();
}

MScene::~MScene()
{
	for (const auto& pair : objMap) {
		delete pair.second;
	}
	objLoader::objReferenceStream.clear();
}

void MScene::addObject(objLoader* m_obj, MObject::Transform m_transform, float m_roughness)
{
	MObject* object = new MObject(m_obj, m_transform, m_roughness);
	objectList.push_back(object);
}

void MScene::addPointLight(glm::vec3 lightPosition, glm::vec3 lightScale, glm::vec3 lightColor, float lightMaxRad)
{
	lightInfos.push_back(lightPosition);
	lightInfos.push_back(lightScale);
	lightInfos.push_back(lightColor);
	lightInfos.push_back(glm::vec3(lightMaxRad));
}

void MScene::setDirectLight(glm::vec3 lightVec, glm::vec3 lightColor)
{
	lightInfos[0] = glm::normalize(lightVec);
	lightInfos[1] = lightColor;
}

void MScene::drawScene(VkCommandBuffer commandBuffer, MPipeline* pipeline, VkPipelineLayout pipelineLayout)
{
	glm::vec3 cameraPos = -invCameraPos;
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &pipeline->descriptorSets, 0, nullptr);
	glm::vec4 NDCCoords[8] = {
		glm::vec4(-1,-1,1,1),
		glm::vec4(1,-1,1,1),
		glm::vec4(-1,1,1,1),
		glm::vec4(1,1,1,1),

		glm::vec4(-1,-1,0,1),
		glm::vec4(1,-1,0,1),
		glm::vec4(-1,1,0,1),
		glm::vec4(1,1,0,1)
	};

	glm::mat4 invP = glm::inverse(proj);
	glm::mat4 invV = glm::inverse(lookat * view);
	glm::vec3 frustumCoords[8];
	for (int i = 0; i < 8; i++) {
		frustumCoords[i] = invV * ((invP * NDCCoords[i]) / (invP * NDCCoords[i]).w);
	}
	//FAR:-direction   0
	//NEAR : direction   4
	//TOP : (1 - 0)x(4 - 0)
	//BOTTOM : (6 - 2)x(3 - 2)
	//LEFT : (4 - 0)x(2 - 0)
	//RIGHT : (3 - 1)x(5 - 1)
	glm::vec3 normals[6];
	float d[6];
	normals[0] = glm::normalize(-cameraDirection);
	normals[1] = glm::normalize(cameraDirection);
	normals[2] = glm::normalize(glm::cross(glm::normalize(frustumCoords[1] - frustumCoords[0]), glm::normalize(cameraPos - frustumCoords[0])));
	normals[3] = glm::normalize(glm::cross(glm::normalize(cameraPos - frustumCoords[2]), glm::normalize(frustumCoords[3] - frustumCoords[2])));
	normals[4] = glm::normalize(glm::cross(glm::normalize(cameraPos - frustumCoords[0]), glm::normalize(frustumCoords[2] - frustumCoords[0])));
	normals[5] = glm::normalize(glm::cross(glm::normalize(frustumCoords[3] - frustumCoords[1]), glm::normalize(cameraPos - frustumCoords[1])));
	d[0] = -glm::dot(normals[0], frustumCoords[0]);
	d[1] = -glm::dot(normals[1], frustumCoords[4]);
	d[2] = -glm::dot(normals[2], cameraPos);
	d[3] = -glm::dot(normals[3], cameraPos);
	d[4] = -glm::dot(normals[4], cameraPos);
	d[5] = -glm::dot(normals[5], cameraPos);
	vector<float>pushConsts;
	for (int i = 0; i < 6; i++) {
		pushConsts.push_back(normals[i].x);
		pushConsts.push_back(normals[i].y);
		pushConsts.push_back(normals[i].z);
		pushConsts.push_back(d[i]);
	}
	pushConsts.push_back(-invCameraPos.x);
	pushConsts.push_back(-invCameraPos.y);
	pushConsts.push_back(-invCameraPos.z);
	pushConsts.push_back(99999);
	const float* VP = glm::value_ptr(proj * lookat * view);
	for (int i = 0; i < 16; i++) {
		pushConsts.push_back(VP[i]);
	}
	if (UIEnable == 1) {
		pushConsts.push_back(jitterBias[currentSubPixel].x);
		pushConsts.push_back(jitterBias[currentSubPixel].y);
	}
	else {
		pushConsts.push_back(0);
		pushConsts.push_back(0);
	}

	//cout << jitterBias[currentSubPixel].x << "   " << jitterBias[currentSubPixel].y << endl;
	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL, 0, PUSH_CONSTS_SIZE, pushConsts.data());
	vkCmdDrawMeshTasksNV(commandBuffer, uint32_t(objectList.size()), 0);
}

void MScene::drawForward(VkCommandBuffer commandBuffer, MPipeline* pipeline, VkPipelineLayout pipelineLayout) {
	uint64_t offsets = 0;
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &pipeline->descriptorSets, 0, nullptr);
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &waterLayer->vertexBuffer, &offsets);
	for (waterLayerInfo info : waterLayerInfos) {
		universalPushConst waterLayerPushConst;
		glm::vec2 warpScale_1 = info.scale / info.warpScale_1;
		glm::vec2 warpScale_2 = info.scale / info.warpScale_2;
		waterLayerPushConst.v4 = glm::vec4(warpScale_1.x, warpScale_1.y, warpScale_2.x, warpScale_2.y);
		waterLayerPushConst.v4_2 = glm::vec4(info.flow_1.x, info.flow_1.y, info.flow_2.x, info.flow_2.y);
		waterLayerPushConst.m4 = glm::translate(glm::mat4(1), info.position) * glm::scale(glm::mat4(1), glm::vec3(info.scale.x, 1, info.scale.y));
		waterLayerPushConst.m4_2 = proj * lookat * view;
		vkCmdPushConstants(commandBuffer, pipeline->universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &waterLayerPushConst);
		vkCmdDraw(commandBuffer, waterLayer->vertexStream.size(), 1, 0, 0);
	}
}

void MScene::createTLAS()
{
	vector<VkAccelerationStructureInstanceKHR> instances;
	for (int i = 0; i < objectList.size(); i++) {
		VkTransformMatrixKHR transformMatrix{};
		for (int j = 0; j < 3; j++) {
			for (int k = 0; k < 4; k++) {
				transformMatrix.matrix[j][k] = objectList[i]->modelMat[k][j];
			}
		}
		VkAccelerationStructureInstanceKHR instance{};
		instance.transform = transformMatrix;
		instance.instanceCustomIndex = 0;
		instance.mask = 0xFF;
		instance.instanceShaderBindingTableRecordOffset = 0;
		instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR;
		instance.accelerationStructureReference = getBufferDeviceAddress(objectList[i]->obj->BLASBuffer);
		instances.push_back(instance);
	}

	void* pInstance = instances.data();

	VkBuffer instancesBuffer;
	VkDeviceMemory instancesMemory;
	createInstanceBuffer(&instancesBuffer, &instancesMemory, &pInstance, objectList.size() * sizeof(VkAccelerationStructureInstanceKHR));

	VkAccelerationStructureGeometryInstancesDataKHR accelerationStructureGeometryInstancesData{};
	accelerationStructureGeometryInstancesData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	accelerationStructureGeometryInstancesData.arrayOfPointers = VK_FALSE;
	accelerationStructureGeometryInstancesData.data.deviceAddress = getBufferDeviceAddress(instancesBuffer);

	VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
	accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	accelerationStructureGeometry.geometry.instances = accelerationStructureGeometryInstancesData;

	VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
	accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationStructureBuildGeometryInfo.geometryCount = 1;
	accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

	uint32_t primitive_count = objectList.size();
	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
	accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	accelerationStructureBuildSizesInfo.pNext = nullptr;
	vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &accelerationStructureBuildGeometryInfo, &primitive_count, &accelerationStructureBuildSizesInfo);

	createBuffer(accelerationStructureBuildSizesInfo.accelerationStructureSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, TLASBuffer, TLASMemory);

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelerationStructureCreateInfo.buffer = TLASBuffer;
	accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
	accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	vkCreateAccelerationStructureKHR(device, &accelerationStructureCreateInfo, nullptr, &TLAS);

	VkBuffer scratchBuffer;
	VkDeviceMemory scratchBufferMemory;
	createBufferWithAddress(accelerationStructureBuildSizesInfo.buildScratchSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, scratchBuffer, scratchBufferMemory);

	accelerationStructureBuildGeometryInfo.dstAccelerationStructure = TLAS;
	accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = getBufferDeviceAddress(scratchBuffer);

	VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
	accelerationStructureBuildRangeInfo.primitiveCount = objectList.size();
	accelerationStructureBuildRangeInfo.primitiveOffset = 0;
	accelerationStructureBuildRangeInfo.firstVertex = 0;
	accelerationStructureBuildRangeInfo.transformOffset = 0;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };
	VkCommandBuffer asCommandBuffer = beginSingleTimeCommands();
	vkCmdBuildAccelerationStructuresKHR(asCommandBuffer, 1, &accelerationStructureBuildGeometryInfo, accelerationBuildStructureRangeInfos.data());
	endSingleTimeCommands(asCommandBuffer);

	MRenderCore::bufferPool.push_back(&TLASBuffer);
	MRenderCore::bufferMemoryPool.push_back(&TLASMemory);
	MRenderCore::asPool.push_back(&TLAS);

	vkDestroyBuffer(device, scratchBuffer, nullptr);
	vkFreeMemory(device, scratchBufferMemory, nullptr);
	vkDestroyBuffer(device, instancesBuffer, nullptr);
	vkFreeMemory(device, instancesMemory, nullptr);
}

void MScene::createSceneVertexBuffer()
{
	vector<float> streamHead;
	vector<objLoader*> objs;
	for (MObject* object : objectList) {
		vector<objLoader*>::iterator it = find(objs.begin(), objs.end(), object->obj);
		int findAt = distance(objs.begin(), it);
		if (findAt >= objs.size()) {
			objs.push_back(object->obj);
			streamHead.push_back(sceneVertexStream.size());
			sceneVertexStream.insert(sceneVertexStream.end(), object->obj->vertexStream.begin(), object->obj->vertexStream.end());
		}
		else {
			streamHead.push_back(streamHead[findAt]);
		}
	}
	for (int i = 0; i < streamHead.size();i++) {
		streamHead[i] += streamHead.size();
	}
	sceneVertexStream.insert(sceneVertexStream.begin(), streamHead.begin(), streamHead.end());
	void* sceneVertexData = sceneVertexStream.data();
	VkDeviceSize sceneVertexBufferSize = sizeof(double) * sceneVertexStream.size();
	createVertexBuffer(&sceneVertexBuffer, &sceneVertexMemory, &sceneVertexData, sceneVertexBufferSize);
}

void MScene::createInstanceStorageBuffer()
{
	for (MObject* object : objectList) {
		instanceStream.push_back(object->obj->beginAt);
		instanceStream.push_back(object->obj->vertexStream.size());
		instanceStream.push_back(object->roughness);
		float* modelMat = glm::value_ptr(object->modelMat);
		for (int i = 0; i < 16; i++) {
			instanceStream.push_back(modelMat[i]);
		}
		instanceStream.push_back(object->obj->aabbMin.x);
		instanceStream.push_back(object->obj->aabbMin.y);
		instanceStream.push_back(object->obj->aabbMin.z);
		instanceStream.push_back(object->obj->aabbMax.x);
		instanceStream.push_back(object->obj->aabbMax.y);
		instanceStream.push_back(object->obj->aabbMax.z);
	}
	void* sceneInstanceData = instanceStream.data();
	VkDeviceSize sceneInstanceBufferSize = sizeof(double) * instanceStream.size();
	createVertexBuffer(&sceneInstanceBuffer, &sceneInstanceMemory, &sceneInstanceData, sceneInstanceBufferSize);
}
