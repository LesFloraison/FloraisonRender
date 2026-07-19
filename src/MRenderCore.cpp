#include "MRenderCore.h"
#include "MCameraTrack.h"
vector<objLoader::Material> MRenderCore::materialArray;
vector<VkImageView> MRenderCore::textureArrayViews;
vector<VkImageView> MRenderCore::cubemapArrayViews;

vector<VkImage*> MRenderCore::imagePool;
vector<VkSampler*> MRenderCore::samplerPool;
vector<VkImageView*> MRenderCore::imageViewPool;
vector<VkDeviceMemory*> MRenderCore::imageMemoryPool;
vector<VkBuffer*> MRenderCore::bufferPool;
vector<VkDeviceMemory*> MRenderCore::bufferMemoryPool;
vector<VkPipeline*> MRenderCore::pipelinePool;
vector<VkAccelerationStructureKHR*> MRenderCore::asPool;

bool freeCam = false;
glm::vec3 invCameraPos;
glm::vec3 historicalInvCameraPos;
glm::vec3 cameraDirection;
glm::mat4 view;
glm::mat4 lookat;
glm::mat4 proj;
glm::mat4 historicalVP;

float QuadVertices[] = {
	 1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 0, 0, 0, 0, 0, 0,
	 1.0f, -1.0f, 0.0f,  1.0f, 0.0f, 0, 0, 0, 0, 0, 0,
	-1.0f,  1.0f, 0.0f,  0.0f, 1.0f, 0, 0, 0, 0, 0, 0,
	 1.0f, -1.0f, 0.0f,  1.0f, 0.0f, 0, 0, 0, 0, 0, 0,
	-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 0, 0,
	-1.0f,  1.0f, 0.0f,  0.0f, 1.0f, 0, 0, 0, 0, 0, 0
};
float cubeVertices[] = {
	-1.0f, -1.0f, -1.0f,  0,0,  0,0,0,  0,0,0,
	 1.0f, -1.0f, -1.0f,  0,0,  0,0,0,  0,0,0,
	 1.0f,  1.0f, -1.0f,  0,0,  0,0,0,  0,0,0,
	 1.0f,  1.0f, -1.0f,  0,0,  0,0,0,  0,0,0,
	-1.0f,  1.0f, -1.0f,  0,0,  0,0,0,  0,0,0,
	-1.0f, -1.0f, -1.0f,  0,0,  0,0,0,  0,0,0,

	-1.0f, -1.0f,  1.0f,  0,0,  0,0,0,  0,0,0,
	 1.0f, -1.0f,  1.0f,  0,0,  0,0,0,  0,0,0,
	 1.0f,  1.0f,  1.0f,  0,0,  0,0,0,  0,0,0,
	 1.0f,  1.0f,  1.0f,  0,0,  0,0,0,  0,0,0,
	-1.0f,  1.0f,  1.0f,  0,0,  0,0,0,  0,0,0,
	-1.0f, -1.0f,  1.0f,  0,0,  0,0,0,  0,0,0,

	-1.0f,  1.0f,  1.0f,  0,0,  0,0,0,  0,0,0,
	-1.0f,  1.0f, -1.0f,  0,0,  0,0,0,  0,0,0,
	-1.0f, -1.0f, -1.0f,  0,0,  0,0,0,  0,0,0,
	-1.0f, -1.0f, -1.0f,  0,0,  0,0,0,  0,0,0,
	-1.0f, -1.0f,  1.0f,  0,0,  0,0,0,  0,0,0,
	-1.0f,  1.0f,  1.0f,  0,0,  0,0,0,  0,0,0,

	 1.0f,  1.0f,  1.0f,  0,0,  0,0,0,  0,0,0,
	 1.0f,  1.0f, -1.0f,  0,0,  0,0,0,  0,0,0,
	 1.0f, -1.0f, -1.0f,  0,0,  0,0,0,  0,0,0,
	 1.0f, -1.0f, -1.0f,  0,0,  0,0,0,  0,0,0,
	 1.0f, -1.0f,  1.0f,  0,0,  0,0,0,  0,0,0,
	 1.0f,  1.0f,  1.0f,  0,0,  0,0,0,  0,0,0,

	-1.0f, -1.0f, -1.0f,  0,0,  0,0,0,  0,0,0,
	 1.0f, -1.0f, -1.0f,  0,0,  0,0,0,  0,0,0,
	 1.0f, -1.0f,  1.0f,  0,0,  0,0,0,  0,0,0,
	 1.0f, -1.0f,  1.0f,  0,0,  0,0,0,  0,0,0,
	-1.0f, -1.0f,  1.0f,  0,0,  0,0,0,  0,0,0,
	-1.0f, -1.0f, -1.0f,  0,0,  0,0,0,  0,0,0,

	-1.0f,  1.0f, -1.0f,  0,0,  0,0,0,  0,0,0,
	 1.0f,  1.0f, -1.0f,  0,0,  0,0,0,  0,0,0,
	 1.0f,  1.0f,  1.0f,  0,0,  0,0,0,  0,0,0,
	 1.0f,  1.0f,  1.0f,  0,0,  0,0,0,  0,0,0,
	-1.0f,  1.0f,  1.0f,  0,0,  0,0,0,  0,0,0,
	-1.0f,  1.0f, -1.0f,  0,0,  0,0,0,  0,0,0
};

string MRenderCore::aspectSelect(string m_selectorPath) {
	std::ifstream file(m_selectorPath);
	std::string line;
	string interfacePath;
	float aspect = float(OUTER_WIDTH) / float(OUTER_HEIGHT);
	if (file.is_open()) {
		while (std::getline(file, line)) {
			if (line[2] == 'a') {
				vector<string> content;
				string subLine = line;
				while (subLine.find(',') != string::npos) {
					int sub1 = subLine.find(':') == (string::npos) ? 9999 : (subLine[subLine.find(':') + 1] == '[' ? subLine.find(':') + 2 : subLine.find(':') + 1);
					int sub2 = subLine[subLine.find(',') + 1] == '"' ? 9999 : subLine.find(',') + 1;
					subLine = subLine.substr(min(sub1, sub2));
					content.push_back(subLine.substr(0, min(min(subLine.find(','), subLine.find(']')), subLine.find('}'))));
				}
				float m_aspect = stof(content[0]);
				interfacePath = content[1].substr(1, content[1].length() - 2);
				cout << abs(aspect - m_aspect) << endl;
				if (abs(aspect - m_aspect) < 0.01) {
					file.close();
					return interfacePath;
				}
			}
		}
	}
	file.close();
	return interfacePath;
}

MRenderCore::MRenderCore(string m_scenePath, string m_interfacePath)
{
	createStorageBuffer(&objLoader::objReferenceBuffer, &objLoader::objReferenceBufferMemory, sizeof(float) * 199000000);
	void* cubeVertexData = (void*)cubeVertices;
	VkDeviceSize cubeVertexBufferSize = sizeof(float) * 396;
	createVertexBuffer(&cubeVertexBuffer, &cubeVertexBufferMemory, &cubeVertexData, cubeVertexBufferSize);
	void* quadVertexData = (void*)QuadVertices;
	VkDeviceSize quadVertexBufferSize = sizeof(float) * 64;
	createVertexBuffer(&quadVertexBuffer, &quadVertexBufferMemory, &quadVertexData, quadVertexBufferSize);

	scene = new MScene(m_scenePath);
	scenePath = m_scenePath;
	interfacePath = m_interfacePath;
	p_interface = new MInterface(m_interfacePath);
	audio = new MAudio(m_scenePath);
	
	axis = new objLoader("res/model/axis/axis.obj");

	createSwapChain();

	createDescriptorSetLayout(&MPipeline::universalDescriptorSetLayout);
	createDescriptorPool(&MPipeline::universalDescriptorPool);
	createPipelineLayout(&MPipeline::universalPipelineLayout, MPipeline::universalDescriptorSetLayout);

	createSkybox("res/textures/darkness");
	//createSkybox("res/textures/kloppenheim_06_puresky_4k");
	//createSkybox("res/textures/Newport_Loft");

	createImage3D(&indirectCache_1, &indirectCacheMemory_1, RADIANCE_CACHE_RAD, RADIANCE_CACHE_RAD, RADIANCE_CACHE_RAD, 1, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	createImageView3D(&indirectCacheView_1, indirectCache_1, 1, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

	createImage3D(&indirectCache_2, &indirectCacheMemory_2, RADIANCE_CACHE_RAD, RADIANCE_CACHE_RAD, RADIANCE_CACHE_RAD, 1, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	createImageView3D(&indirectCacheView_2, indirectCache_2, 1, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

	createImage(&gHistoricalDirect, &gHistoricalDirectMemory, INNER_WIDTH, INNER_HEIGHT, 1, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	createImageView(&gHistoricalDirectView, gHistoricalDirect, 1, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

	createImage(&gHistoricalIndirect, &gHistoricalIndirectMemory, INNER_WIDTH, INNER_HEIGHT, 1, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	createImageView(&gHistoricalIndirectView, gHistoricalIndirect, 1, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

	createImage(&gHistoricalTaau, &gHistoricalTaauMemory, INNER_WIDTH * 2, INNER_HEIGHT * 2, 1, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	createImageView(&gHistoricalTaauView, gHistoricalTaau, 1, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

	createImage(&gHistoricalTaauPosition, &gHistoricalTaauPositionMemory, INNER_WIDTH * 2, INNER_HEIGHT * 2, 1, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	createImageView(&gHistoricalTaauPositionView, gHistoricalTaauPosition, 1, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

	createImage(&gHistoricalTaauNormal, &gHistoricalTaauNormalMemory, INNER_WIDTH * 2, INNER_HEIGHT * 2, 1, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	createImageView(&gHistoricalTaauNormalView, gHistoricalTaauNormal, 1, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

	transitionImageLayout(gHistoricalDirect, 1, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	transitionImageLayout(gHistoricalIndirect, 1, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	transitionImageLayout(gHistoricalTaau, 1, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	transitionImageLayout(gHistoricalTaauPosition, 1, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	transitionImageLayout(gHistoricalTaauNormal, 1, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	float sphereSamples[(128 + 64) * 3];

	for (int i = 0; i < 128; i++) {
		float x = ((rand() % 1000) / 1000.000) * 2 - 1;
		float y = ((rand() % 1000) / 1000.000) * 2 - 1;
		float z = ((rand() % 1000) / 1000.000);
		glm::vec3 sample = glm::normalize(glm::vec3(x, y, z));
		sphereSamples[3 * i + 0] = sample.x;
		sphereSamples[3 * i + 1] = sample.y;
		sphereSamples[3 * i + 2] = sample.z;
	}

	for (int i = 128; i < 128 + 64; i++) {
		float x = ((rand() % 1000) / 1000.000) * 2 - 1;
		float y = ((rand() % 1000) / 1000.000) * 2 - 1;
		float z = ((rand() % 1000) / 1000.000) * 2 - 1;
		glm::vec3 sample = glm::normalize(glm::vec3(x, y, z));
		sphereSamples[3 * i + 0] = sample.x;
		sphereSamples[3 * i + 1] = sample.y;
		sphereSamples[3 * i + 2] = sample.z;
	}

	void* pSphereSamples = sphereSamples;
	createVertexBuffer(&sampleStorageBuffer, &sampleStorageMemory, &pSphereSamples, sizeof(sphereSamples));


	vector<VkFormat> geometryFormats = { VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT };
	geometryPipeline = new MPipeline(0, "shaders/geometry.task", "shaders/geometry.mesh", "shaders/geometry.frag", 3);
	geometryPipeline->colorAttachmentFormats = geometryFormats;
	geometryPipeline->pVertexBuffer = &objLoader::objReferenceBuffer;
	geometryPipeline->pStorageBuffer = &scene->sceneInstanceBuffer;
	geometryPipeline->image2DViews = textureArrayViews;
	geometryPipeline->TLAS = &scene->TLAS;
	geometryPipeline->TLASBuffer = &scene->TLASBuffer;
	geometryPipeline->createPipeline();

	UILayerPipeline = new MPipeline("shaders/UI.vert", "shaders/UI.frag", 1);
	UILayerPipeline->image2DViews = textureArrayViews;
	UILayerPipeline->TLAS = &scene->TLAS;
	UILayerPipeline->TLASBuffer = &scene->TLASBuffer;
	UILayerPipeline->createPipeline();
	skyboxSamplerPipeline = new MPipeline("shaders/skyboxSampler.vert", "shaders/skyboxSampler.frag", 1);
	skyboxSamplerPipeline->image2DViews = textureArrayViews;
	skyboxSamplerPipeline->imageCubeViews = cubemapArrayViews;
	skyboxSamplerPipeline->TLAS = &scene->TLAS;
	skyboxSamplerPipeline->TLASBuffer = &scene->TLASBuffer;
	skyboxSamplerPipeline->createPipeline();

	vector<VkImageView> cacheViewerResource;
	cacheViewerResource.push_back(geometryPipeline->colorAttachmentViews[0]);
	cacheViewerResource.push_back(geometryPipeline->colorAttachmentViews[1]);
	cacheViewerPipeline = new MPipeline("shaders/screen.vert", "shaders/cacheViewer.frag", 2);
	cacheViewerPipeline->image2DViews = cacheViewerResource;
	cacheViewerPipeline->TLAS = &scene->TLAS;
	cacheViewerPipeline->TLASBuffer = &scene->TLASBuffer;
	cacheViewerPipeline->indirectCacheView_1 = indirectCacheView_1;
	cacheViewerPipeline->indirectCacheView_2 = indirectCacheView_2;
	cacheViewerPipeline->createPipeline();

	vector<VkImageView> deferredResource = geometryPipeline->colorAttachmentViews;
	deferredResource.push_back(gHistoricalTaauPositionView);
	deferredResource.push_back(gHistoricalTaauNormalView);
	deferredResource.push_back(gHistoricalDirectView);
	deferredResource.push_back(gHistoricalIndirectView);
	deferredResource.push_back(cacheViewerPipeline->colorAttachmentViews[1]);
	deferredResource.insert(deferredResource.end(), textureArrayViews.begin(), textureArrayViews.end());
	deferredPipeline = new MPipeline("shaders/screen.vert", "shaders/deferred.frag", 2);
	deferredPipeline->image2DViews = deferredResource;
	deferredPipeline->pVertexBuffer = &scene->sceneVertexBuffer;
	deferredPipeline->pStorageBuffer = &sampleStorageBuffer;
	deferredPipeline->imageCubeViews = cubemapArrayViews;
	deferredPipeline->TLAS = &scene->TLAS;
	deferredPipeline->TLASBuffer = &scene->TLASBuffer;
	deferredPipeline->indirectCacheView_1 = indirectCacheView_1;
	deferredPipeline->indirectCacheView_2 = indirectCacheView_2;
	deferredPipeline->createPipeline();

	vector<VkImageView> injectorResource;
	injectorResource.push_back(geometryPipeline->colorAttachmentViews[0]);
	injectorResource.push_back(geometryPipeline->colorAttachmentViews[1]);
	injectorResource.push_back(deferredPipeline->colorAttachmentViews[1]);
	injectorPipeline = new MPipeline("shaders/screen.vert", "shaders/injector.frag", 1);
	injectorPipeline->image2DViews = injectorResource;
	injectorPipeline->TLAS = &scene->TLAS;
	injectorPipeline->TLASBuffer = &scene->TLASBuffer;
	injectorPipeline->indirectCacheView_1 = indirectCacheView_1;
	injectorPipeline->indirectCacheView_2 = indirectCacheView_2;
	injectorPipeline->createPipeline();

	vector<VkImageView> preFilterResource = geometryPipeline->colorAttachmentViews;
	preFilterResource.push_back(deferredPipeline->colorAttachmentViews[0]);
	preFilterResource.push_back(deferredPipeline->colorAttachmentViews[1]);
	preFilterPipeline = new MPipeline("shaders/screen.vert", "shaders/preFilter.frag", 2);
	preFilterPipeline->image2DViews = preFilterResource;
	preFilterPipeline->TLAS = &scene->TLAS;
	preFilterPipeline->TLASBuffer = &scene->TLASBuffer;
	preFilterPipeline->createPipeline();

	vector<VkImageView> filterResource = geometryPipeline->colorAttachmentViews;
	filterResource.push_back(preFilterPipeline->colorAttachmentViews[0]);
	filterResource.push_back(preFilterPipeline->colorAttachmentViews[1]);
	filterPipeline = new MPipeline("shaders/screen.vert", "shaders/filter.frag", 1);
	filterPipeline->image2DViews = filterResource;
	filterPipeline->TLAS = &scene->TLAS;
	filterPipeline->TLASBuffer = &scene->TLASBuffer;
	filterPipeline->createPipeline();


	waterLayerPipeline = new MPipeline("shaders/waterLayer.vert", "shaders/waterLayer.frag", 1);
	waterLayerPipeline->image2DViews[0] = geometryPipeline->colorAttachmentViews[0];
	waterLayerPipeline->image2DViews.push_back(filterPipeline->colorAttachmentViews[0]);
	waterLayerPipeline->image2DViews.insert(waterLayerPipeline->image2DViews.end(), textureArrayViews.begin(), textureArrayViews.end());
	waterLayerPipeline->depthView = geometryPipeline->depthView;
	waterLayerPipeline->depthAttachmentLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	waterLayerPipeline->pVertexBuffer = &scene->sceneVertexBuffer;
	waterLayerPipeline->pStorageBuffer = &sampleStorageBuffer;
	waterLayerPipeline->imageCubeViews = cubemapArrayViews;
	waterLayerPipeline->TLAS = &scene->TLAS;
	waterLayerPipeline->TLASBuffer = &scene->TLASBuffer;
	waterLayerPipeline->createPipeline();

	vector<VkFormat> taauFormats = { VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT };
	vector<VkImageView> taauResource;
	taauResource.push_back(geometryPipeline->colorAttachmentViews[0]);
	taauResource.push_back(geometryPipeline->colorAttachmentViews[1]);
	taauResource.push_back(filterPipeline->colorAttachmentViews[0]);
	taauResource.push_back(gHistoricalTaauView);
	taauResource.push_back(gHistoricalTaauPositionView);
	taauResource.push_back(gHistoricalTaauNormalView);
	taauPipeline = new MPipeline("shaders/screen.vert", "shaders/taau.frag", 3);
	taauPipeline->colorAttachmentFormats = taauFormats;
	taauPipeline->image2DViews = taauResource;
	taauPipeline->pipelineWidth = INNER_WIDTH * 2;
	taauPipeline->pipelineHeight = INNER_HEIGHT * 2;
	taauPipeline->TLAS = &scene->TLAS;
	taauPipeline->TLASBuffer = &scene->TLASBuffer;
	taauPipeline->createPipeline();

	vector<VkImageView> assembleResource;
	assembleResource.push_back(taauPipeline->colorAttachmentViews[0]);
	assembleResource.push_back(skyboxSamplerPipeline->colorAttachmentViews[0]);
	assembleResource.push_back(waterLayerPipeline->colorAttachmentViews[0]);
	assemblePipeline = new MPipeline("shaders/screen.vert", "shaders/assemble.frag", 1);
	assemblePipeline->image2DViews = assembleResource;
	assemblePipeline->pipelineWidth = INNER_WIDTH * 2;
	assemblePipeline->pipelineHeight = INNER_HEIGHT * 2;
	assemblePipeline->TLAS = &scene->TLAS;
	assemblePipeline->TLASBuffer = &scene->TLASBuffer;
	assemblePipeline->createPipeline();

	vector<VkImageView> easuResource = assemblePipeline->colorAttachmentViews;
	easuPipeline = new MPipeline("shaders/screen.vert", "shaders/easu.frag", 1);
	easuPipeline->image2DViews = easuResource;
	easuPipeline->pipelineWidth = OUTER_WIDTH;
	easuPipeline->pipelineHeight = OUTER_HEIGHT;
	easuPipeline->TLAS = &scene->TLAS;
	easuPipeline->TLASBuffer = &scene->TLASBuffer;
	easuPipeline->createPipeline();

	vector<VkImageView> rcasResource = easuPipeline->colorAttachmentViews;
	rcasPipeline = new MPipeline("shaders/screen.vert", "shaders/rcas.frag", 1);
	rcasPipeline->image2DViews = rcasResource;
	rcasPipeline->pipelineWidth = OUTER_WIDTH;
	rcasPipeline->pipelineHeight = OUTER_HEIGHT;
	rcasPipeline->TLAS = &scene->TLAS;
	rcasPipeline->TLASBuffer = &scene->TLASBuffer;
	rcasPipeline->createPipeline();

	vector<VkImageView> interfacePreResource = rcasPipeline->colorAttachmentViews;
	interfacePreResource.insert(interfacePreResource.end(), MInterface::interfaceTextureArrayViews.begin(), MInterface::interfaceTextureArrayViews.end());
	interfacePrePipeline = new MPipeline("shaders/interfacePre.vert", "shaders/interfacePre.frag", 2);
	interfacePrePipeline->image2DViews = interfacePreResource;
	interfacePrePipeline->pipelineWidth = OUTER_WIDTH;
	interfacePrePipeline->pipelineHeight = OUTER_HEIGHT;
	interfacePrePipeline->TLAS = &scene->TLAS;
	interfacePrePipeline->TLASBuffer = &scene->TLASBuffer;
	interfacePrePipeline->createPipeline();

	vector<VkImageView> interfaceResource = interfacePrePipeline->colorAttachmentViews;
	interfacePipeline = new MPipeline("shaders/screen.vert", "shaders/interface.frag", 1);
	interfacePipeline->image2DViews = interfaceResource;
	interfacePipeline->pipelineWidth = OUTER_WIDTH;
	interfacePipeline->pipelineHeight = OUTER_HEIGHT;
	interfacePipeline->TLAS = &scene->TLAS;
	interfacePipeline->TLASBuffer = &scene->TLASBuffer;
	interfacePipeline->createPipeline();

	fontPipeline = new MPipeline("shaders/font.vert", "shaders/font.frag", 1);
	fontPipeline->image2DViews = MInterface::fontTextureArrayViews;
	fontPipeline->pipelineWidth = OUTER_WIDTH;
	fontPipeline->pipelineHeight = OUTER_HEIGHT;
	fontPipeline->TLAS = &scene->TLAS;
	fontPipeline->TLASBuffer = &scene->TLASBuffer;
	fontPipeline->createPipeline();

	vector<VkImageView> frame0Resource = geometryPipeline->colorAttachmentViews;
	frame0Resource.push_back(skyboxSamplerPipeline->colorAttachmentViews[0]);
	frame0Resource.push_back(UILayerPipeline->colorAttachmentViews[0]);
	frame0Resource.push_back(filterPipeline->colorAttachmentViews[0]);
	frame0Resource.push_back(easuPipeline->colorAttachmentViews[0]);
	frame0Resource.push_back(rcasPipeline->colorAttachmentViews[0]);
	frame0Resource.push_back(taauPipeline->colorAttachmentViews[0]);
	frame0Resource.push_back(taauPipeline->colorAttachmentViews[1]);
	frame0Resource.push_back(taauPipeline->colorAttachmentViews[2]);
	frame0Resource.push_back(cacheViewerPipeline->colorAttachmentViews[0]);
	frame0Resource.push_back(cacheViewerPipeline->colorAttachmentViews[1]);
	frame0Resource.push_back(deferredPipeline->colorAttachmentViews[0]);
	frame0Resource.push_back(deferredPipeline->colorAttachmentViews[1]);
	frame0Resource.push_back(waterLayerPipeline->colorAttachmentViews[0]);
	frame0Resource.push_back(interfacePipeline->colorAttachmentViews[0]);
	frame0Resource.push_back(assemblePipeline->colorAttachmentViews[0]);
	frame0Resource.push_back(fontPipeline->colorAttachmentViews[0]);
	frame0Pipeline = new MPipeline("shaders/screen.vert", "shaders/frame0.frag");
	frame0Pipeline->image2DViews = frame0Resource;
	frame0Pipeline->pipelineWidth = OUTER_WIDTH;
	frame0Pipeline->pipelineHeight = OUTER_HEIGHT;
	frame0Pipeline->TLAS = &scene->TLAS;
	frame0Pipeline->TLASBuffer = &scene->TLASBuffer;
	frame0Pipeline->createPipeline();

	transitionImageLayout(deferredPipeline->colorAttachmentImages[0], 1, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	transitionImageLayout(deferredPipeline->colorAttachmentImages[1], 1, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	transitionImageLayout(taauPipeline->colorAttachmentImages[0], 1, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	transitionImageLayout(taauPipeline->colorAttachmentImages[1], 1, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	transitionImageLayout(taauPipeline->colorAttachmentImages[2], 1, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	renderPasses = createDefaultRenderPasses();
}

MRenderCore::~MRenderCore()
{
	renderPasses.clear();

	for (VkAccelerationStructureKHR* accelerationStructure : MRenderCore::asPool) {
		vkDestroyAccelerationStructureKHR(device, *accelerationStructure, nullptr);
	}
	for (VkImageView* imageView : MRenderCore::imageViewPool) {
		vkDestroyImageView(device, *imageView, nullptr);
	}
	for (VkImage* image : MRenderCore::imagePool) {
		vkDestroyImage(device, *image, nullptr);
	}
	for (VkDeviceMemory* imageMemory : MRenderCore::imageMemoryPool) {
		vkFreeMemory(device, *imageMemory, nullptr);
	}
	for (VkSampler* sampler : MRenderCore::samplerPool) {
		vkDestroySampler(device, *sampler, nullptr);
	}

	vkDestroyDescriptorPool(device, MPipeline::universalDescriptorPool, nullptr);
	for (VkBuffer* buffer : MRenderCore::bufferPool) {
		vkDestroyBuffer(device, *buffer, nullptr);
	}

	for (VkDeviceMemory* bufferMemory : MRenderCore::bufferMemoryPool) {
		vkFreeMemory(device, *bufferMemory, nullptr);
	}
	vkDestroyDescriptorSetLayout(device, MPipeline::universalDescriptorSetLayout, nullptr);
	for (VkPipeline* pipeline : MRenderCore::pipelinePool) {
		vkDestroyPipeline(device, *pipeline, nullptr);
	}
	vkDestroyPipelineLayout(device, MPipeline::universalPipelineLayout, nullptr);

	for (auto framebuffer : swapChainFramebuffers) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}
	vkDestroySwapchainKHR(device, swapChain, nullptr);


	materialArray.clear();
	textureArrayViews.clear();
	cubemapArrayViews.clear();
	MInterface::interfaceTextureArrayViews.clear();
	MInterface::fontTextureArrayViews.clear();
	MInterface::textDisableTable.assign(MInterface::textDisableTable.size(), 0);

	imagePool.clear();
	samplerPool.clear();
	imageViewPool.clear();
	imageMemoryPool.clear();
	bufferPool.clear();
	bufferMemoryPool.clear();
	pipelinePool.clear();
	asPool.clear();

	delete scene;
	delete p_interface;
	delete audio;
}

void MRenderCore::updateUniform()
{
	view = glm::translate(glm::mat4(1), invCameraPos);
	lookat = glm::lookAt(glm::vec3(0), cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f));
	proj = glm::perspective(glm::radians(FOV), swapChainExtent.width / (float)swapChainExtent.height, NEAR_PLANE, FAR_PLANE);
	proj[1][1] *= -1;

	deferredUniformBuffer deferredUbo;
	deferredUbo.cameraPos = -invCameraPos;
	deferredUbo.historicalVP = historicalVP;
	deferredUbo.runingTime = runingTime;
	deferredUbo.randSeed = (rand() % 1000) / 10.0 + 2;
	deferredUbo.pointLightSize = (scene->lightInfos.size() - 2) / 3;
	deferredUbo.debugVal = debugVal;
	for (int i = 0; i < scene->lightInfos.size(); i++) {
		deferredUbo.lightInfos[i] = glm::vec4(scene->lightInfos[i], 0);
	}
	updateUniformBuffer(deferredPipeline->uniformBuffersMapped, &deferredUbo);
	updateUniformBuffer(waterLayerPipeline->uniformBuffersMapped, &deferredUbo);
}

void MRenderCore::drawFrame()
{
	cul_mouseDir(&cameraDirection);
	if (MCameraTrack::isTracking) {
		invCameraPos = MCameraTrack::MCTinvCameraPos;
		cameraDirection = MCameraTrack::MCTcameraDirection;
	}
	updateUniform();
	p_interface->executionTrigger();
	uint32_t imageIndex;
	vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores, VK_NULL_HANDLE, &imageIndex);

	RenderFrameContext frameContext{ *this, imageIndex };
	for (const auto& pass : renderPasses) {
		pass->execute(frameContext);
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 0;
	presentInfo.pWaitSemaphores = nullptr;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapChain;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	vkQueuePresentKHR(graphicsPresentQueue, &presentInfo);

	copyImage2D(&gHistoricalDirect, &deferredPipeline->colorAttachmentImages[0], INNER_WIDTH, INNER_HEIGHT);
	copyImage2D(&gHistoricalIndirect, &deferredPipeline->colorAttachmentImages[1], INNER_WIDTH, INNER_HEIGHT);
	copyImage2D(&gHistoricalTaau, &taauPipeline->colorAttachmentImages[0], INNER_WIDTH * 2, INNER_HEIGHT * 2);
	copyImage2D(&gHistoricalTaauPosition, &taauPipeline->colorAttachmentImages[1], INNER_WIDTH * 2, INNER_HEIGHT * 2);
	copyImage2D(&gHistoricalTaauNormal, &taauPipeline->colorAttachmentImages[2], INNER_WIDTH * 2, INNER_HEIGHT * 2);
	glm::mat4 historicalP = glm::perspective(glm::radians(FOV), swapChainExtent.width / (float)swapChainExtent.height, NEAR_PLANE, FAR_PLANE);
	historicalP[1][1] *= -1;
	historicalVP = historicalP * glm::lookAt(glm::vec3(0), cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1), invCameraPos);
	historicalInvCameraPos = invCameraPos;

	currentSubPixel = (currentSubPixel + 1) % 4;
}
