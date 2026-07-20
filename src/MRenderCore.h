#pragma once
#include "MPipeline.h"
#include "MScene.h"
#include "MInterface.h"
#include "MAudio.h"
#include "MRenderPass.h"
class MRenderCore
{
public:
	MScene* scene;
	MInterface* p_interface;
	MAudio* audio;
	objLoader* axis;

	string scenePath;
	string interfacePath;

	std::vector<std::unique_ptr<IRenderPass>> renderPasses;
	FrameHistorySources frameHistorySources;

	static vector<objLoader::Material> materialArray;
	static vector<VkImageView> textureArrayViews;
	static vector<VkImageView> cubemapArrayViews;

	static vector<VkImage*> imagePool;
	static vector<VkSampler*> samplerPool;
	static vector<VkImageView*> imageViewPool;
	static vector<VkDeviceMemory*> imageMemoryPool;
	static vector<VkBuffer*> bufferPool;
	static vector<VkDeviceMemory*> bufferMemoryPool;
	static vector<VkPipeline*> pipelinePool;
	static vector<VkAccelerationStructureKHR*> asPool;

	VkDeviceMemory cubeVertexBufferMemory;
	VkBuffer cubeVertexBuffer;
	VkDeviceMemory quadVertexBufferMemory;
	VkBuffer quadVertexBuffer;

	VkImage gHistoricalTaauPosition;
	VkImageView gHistoricalTaauPositionView;
	VkDeviceMemory gHistoricalTaauPositionMemory;
	VkImage gHistoricalTaauNormal;
	VkImageView gHistoricalTaauNormalView;
	VkDeviceMemory gHistoricalTaauNormalMemory;
	VkImage gHistoricalDirect;
	VkImageView gHistoricalDirectView;
	VkDeviceMemory gHistoricalDirectMemory;
	VkImage gHistoricalIndirect;
	VkImageView gHistoricalIndirectView;
	VkDeviceMemory gHistoricalIndirectMemory;
	VkImage gHistoricalTaau;
	VkImageView gHistoricalTaauView;
	VkDeviceMemory gHistoricalTaauMemory;
	VkImage indirectCache_1;
	VkImageView indirectCacheView_1;
	VkDeviceMemory indirectCacheMemory_1;
	VkImage indirectCache_2;
	VkImageView indirectCacheView_2;
	VkDeviceMemory indirectCacheMemory_2;

	VkBuffer sampleStorageBuffer;
	VkDeviceMemory sampleStorageMemory;

	MRenderCore(string m_scenePath, string m_interfacePath);
	static string aspectSelect(string m_selectorPath);
	~MRenderCore();
	deferredUniformBuffer updateUniform();
	void drawFrame();
};

