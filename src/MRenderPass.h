#pragma once

#include "encapVk.h"

#include <memory>
#include <string>
#include <vector>

class MRenderCore;

struct RenderFrameContext
{
	MRenderCore& renderCore;
	uint32_t swapchainImageIndex;
	VkDeviceSize vertexBufferOffset = 0;
};

class IRenderPass
{
public:
	explicit IRenderPass(std::string name);
	virtual ~IRenderPass();

	IRenderPass(const IRenderPass&) = delete;
	IRenderPass& operator=(const IRenderPass&) = delete;

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

std::vector<std::unique_ptr<IRenderPass>> createDefaultRenderPasses();
