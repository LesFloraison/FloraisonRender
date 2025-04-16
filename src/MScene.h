#pragma once
#include "encapVk.h"
#include "objLoader.h"
#include "MObject.h"
#include "JSON.h"
class MScene
{
public:
	struct pointLight {
		glm::vec3 position;
		glm::vec3 light;
	};
	struct waterLayerInfo {
		glm::vec3 position;
		glm::vec2 scale;
		float warpScale_1;
		float warpScale_2;
		glm::vec2 flow_1;
		glm::vec2 flow_2;
	};
	map<int, objLoader*> objMap;
	vector<MObject*> objectList;
	vector<glm::vec3> lightInfos;
	vector<waterLayerInfo> waterLayerInfos;
	VkBuffer TLASBuffer;
	VkDeviceMemory TLASMemory;
	VkAccelerationStructureKHR TLAS;
	vector<double> sceneVertexStream;
	vector<double> instanceStream;
	VkBuffer sceneVertexBuffer;
	VkDeviceMemory sceneVertexMemory;
	VkBuffer sceneReferenceBuffer;
	VkDeviceMemory sceneReferenceMemory;
	VkBuffer sceneInstanceBuffer;
	VkDeviceMemory sceneInstanceMemory;
	objLoader* waterLayer;
	MScene(string path);
	~MScene();
	void addObject(objLoader* m_obj, MObject::Transform m_transformInfo, float roughness);
	void addPointLight(glm::vec3 lightPosition, glm::vec3 lightScale, glm::vec3 lightColor, float lightMaxRad);
	void setDirectLight(glm::vec3 lightVec, glm::vec3 lightColor);
	void drawScene(VkCommandBuffer commandBuffer, MPipeline* pipeline, VkPipelineLayout pipelineLayout);
	void drawForward(VkCommandBuffer commandBuffer, MPipeline* pipeline, VkPipelineLayout pipelineLayout);
	void createTLAS();
	void createSceneVertexBuffer();
	void createInstanceStorageBuffer();
};

