#include "MRenderCore.h"
#include "MCameraTrack.h"
#include <utility>
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


	RenderSetupContext setupContext{
		*scene,
		*p_interface,
		*axis,
		cubeVertexBuffer,
		quadVertexBuffer,
		sampleStorageBuffer,
		textureArrayViews,
		cubemapArrayViews,
		{
			gHistoricalDirectView,
			gHistoricalIndirectView,
			gHistoricalTaauView,
			gHistoricalTaauPositionView,
			gHistoricalTaauNormalView
		},
		{ indirectCacheView_1, indirectCacheView_2 }
	};

	RenderPassBuildResult passBuildResult = createDefaultRenderPasses(setupContext);
	frameHistorySources = passBuildResult.historySources;
	renderPasses = std::move(passBuildResult.passes);
}

MRenderCore::~MRenderCore()
{
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

	// MPipeline handles stored in the pools above point into pass-owned objects,
	// so the pass container must outlive Vulkan handle destruction.
	renderPasses.clear();

	delete scene;
	delete p_interface;
	delete audio;
}

deferredUniformBuffer MRenderCore::updateUniform()
{
	view = glm::translate(glm::mat4(1), invCameraPos);
	lookat = glm::lookAt(glm::vec3(0), cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f));
	proj = glm::perspective(glm::radians(FOV), swapChainExtent.width / (float)swapChainExtent.height, NEAR_PLANE, FAR_PLANE);
	proj[1][1] *= -1;

	deferredUniformBuffer deferredUbo{};
	deferredUbo.cameraPos = -invCameraPos;
	deferredUbo.historicalVP = historicalVP;
	deferredUbo.runingTime = runingTime;
	deferredUbo.randSeed = (rand() % 1000) / 10.0 + 2;
	deferredUbo.pointLightSize = (scene->lightInfos.size() - 2) / 3;
	deferredUbo.debugVal = debugVal;
	for (int i = 0; i < scene->lightInfos.size(); i++) {
		deferredUbo.lightInfos[i] = glm::vec4(scene->lightInfos[i], 0);
	}
	return deferredUbo;
}

void MRenderCore::drawFrame()
{
	cul_mouseDir(&cameraDirection);
	if (MCameraTrack::isTracking) {
		invCameraPos = MCameraTrack::MCTinvCameraPos;
		cameraDirection = MCameraTrack::MCTcameraDirection;
	}
	deferredUniformBuffer sceneUniform = updateUniform();
	p_interface->executionTrigger();
	uint32_t imageIndex;
	vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores, VK_NULL_HANDLE, &imageIndex);

	RenderFrameContext frameContext{ imageIndex, 0, &sceneUniform };
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

	copyImage2D(&gHistoricalDirect, &frameHistorySources.direct.image, INNER_WIDTH, INNER_HEIGHT);
	copyImage2D(&gHistoricalIndirect, &frameHistorySources.indirect.image, INNER_WIDTH, INNER_HEIGHT);
	copyImage2D(&gHistoricalTaau, &frameHistorySources.taau.image, INNER_WIDTH * 2, INNER_HEIGHT * 2);
	copyImage2D(&gHistoricalTaauPosition, &frameHistorySources.taauPosition.image, INNER_WIDTH * 2, INNER_HEIGHT * 2);
	copyImage2D(&gHistoricalTaauNormal, &frameHistorySources.taauNormal.image, INNER_WIDTH * 2, INNER_HEIGHT * 2);
	glm::mat4 historicalP = glm::perspective(glm::radians(FOV), swapChainExtent.width / (float)swapChainExtent.height, NEAR_PLANE, FAR_PLANE);
	historicalP[1][1] *= -1;
	historicalVP = historicalP * glm::lookAt(glm::vec3(0), cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1), invCameraPos);
	historicalInvCameraPos = invCameraPos;

	currentSubPixel = (currentSubPixel + 1) % 4;
}
