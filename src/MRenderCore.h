#pragma once
#include "MPipeline.h"
#include "MScene.h"
#include "MInterface.h"
#define GEOMETRY_PASS 0
#define CACHE_VIEWER_PASS 1
#define DEFERRED_PASS 2
#define INJECTOR_PASS 3
#define PREFILTER_PASS 4
#define FILTER_PASS 5
#define FORWARD_PASS 6
#define TAAU_PASS 7
#define ASSEMBLE_PASS 8
#define EASU_PASS 9
#define RCAS_PASS 10
#define INTERFACEPRE_PASS 11
#define INTERFACE_PASS 12
#define FONT_PASS 13
#define FRAME0_PASS 14
class MRenderCore
{
public:
	MScene* scene;
	MInterface* p_interface;
	objLoader* axis;

	string scenePath;
	string interfacePath;

	MPipeline* geometryPipeline;
	MPipeline* skyboxSamplerPipeline;
	MPipeline* UILayerPipeline;
	MPipeline* deferredPipeline;
	MPipeline* injectorPipeline;
	MPipeline* cacheViewerPipeline;
	MPipeline* preFilterPipeline;
	MPipeline* filterPipeline;
	MPipeline* waterLayerPipeline;
	MPipeline* taauPipeline;
	MPipeline* assemblePipeline;
	MPipeline* easuPipeline;
	MPipeline* rcasPipeline;
	MPipeline* interfacePrePipeline;
	MPipeline* interfacePipeline;
	MPipeline* fontPipeline;
	MPipeline* frame0Pipeline;
	vector<VkCommandBuffer*> pCmdBuffers;

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
	void updateUniform();
	void drawFrame();
};

