#pragma once

#include "encapVk.h"

#include <memory>
#include <string>
#include <vector>

class MScene;
class MInterface;
class objLoader;

struct RenderTexture
{
	VkImage image = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
};

struct TemporalHistoryResources
{
	VkImageView direct = VK_NULL_HANDLE;
	VkImageView indirect = VK_NULL_HANDLE;
	VkImageView taau = VK_NULL_HANDLE;
	VkImageView taauPosition = VK_NULL_HANDLE;
	VkImageView taauNormal = VK_NULL_HANDLE;
};

struct RadianceCacheResources
{
	VkImageView first = VK_NULL_HANDLE;
	VkImageView second = VK_NULL_HANDLE;
};

struct RenderSetupContext
{
	MScene& scene;
	MInterface& interfaceLayer;
	objLoader& axis;
	VkBuffer& cubeVertexBuffer;
	VkBuffer& quadVertexBuffer;
	VkBuffer& sampleStorageBuffer;
	std::vector<VkImageView>& materialTextures;
	std::vector<VkImageView>& cubemaps;
	TemporalHistoryResources history;
	RadianceCacheResources radianceCache;
};

struct RenderFrameContext
{
	uint32_t swapchainImageIndex;
	VkDeviceSize vertexBufferOffset = 0;
	const deferredUniformBuffer* sceneUniform = nullptr;
};

struct FrameHistorySources
{
	RenderTexture direct;
	RenderTexture indirect;
	RenderTexture taau;
	RenderTexture taauPosition;
	RenderTexture taauNormal;
};

class IRenderPass
{
public:
	explicit IRenderPass(std::string name);
	virtual ~IRenderPass();

	IRenderPass(const IRenderPass&) = delete;
	IRenderPass& operator=(const IRenderPass&) = delete;

	virtual void setup(RenderSetupContext& context) = 0;
	virtual void execute(RenderFrameContext& context) = 0;

	const std::string& name() const;

protected:
	VkCommandBuffer commandBuffer() const;
	void beginRecording();
	void submit(VkSemaphore* waitSemaphore = nullptr, VkSemaphore* signalSemaphore = nullptr);

private:
	std::string passName;
	VkCommandBuffer passCommandBuffer = VK_NULL_HANDLE;
};

struct RenderPassBuildResult
{
	std::vector<std::unique_ptr<IRenderPass>> passes;
	FrameHistorySources historySources;
};

RenderPassBuildResult createDefaultRenderPasses(RenderSetupContext& context);
