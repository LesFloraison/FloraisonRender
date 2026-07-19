#include "MRenderPass.h"

#include "MRenderCore.h"

#include <utility>

IRenderPass::IRenderPass(std::string name)
	: passName(std::move(name))
{
	createCommandBuffer(&passCommandBuffer);
}

IRenderPass::~IRenderPass()
{
	if (passCommandBuffer != VK_NULL_HANDLE) {
		vkFreeCommandBuffers(device, commandPool, 1, &passCommandBuffer);
	}
}

const std::string& IRenderPass::name() const
{
	return passName;
}

VkCommandBuffer IRenderPass::commandBuffer() const
{
	return passCommandBuffer;
}

void IRenderPass::beginRecording()
{
	// Keep the existing synchronization behavior during this first refactor.
	vkQueueWaitIdle(graphicsPresentQueue);
	beginRecord(&passCommandBuffer);
}

void IRenderPass::submit(VkSemaphore* waitSemaphore, VkSemaphore* signalSemaphore)
{
	endRecordSubmit(&passCommandBuffer, waitSemaphore, signalSemaphore);
}

namespace
{
	class GeometryPass final : public IRenderPass
	{
	public:
		GeometryPass() : IRenderPass("Geometry") {}

		void execute(RenderFrameContext& context) override
		{
			auto& core = context.renderCore;
			VkDeviceSize offsets = context.vertexBufferOffset;

			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &core.skyboxSamplerPipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, core.skyboxSamplerPipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &core.skyboxSamplerPipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, &core.cubeVertexBuffer, &offsets);
			glm::mat4 P_lookat = proj * lookat;
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, PUSH_CONSTS_SIZE, glm::value_ptr(P_lookat));
			vkCmdDraw(commandBuffer(), 36, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());

			vkCmdBeginRendering(commandBuffer(), &core.UILayerPipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, core.UILayerPipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &core.UILayerPipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, &core.axis->vertexBuffer, &offsets);
			glm::mat4 UIMVP = proj * lookat * glm::translate(glm::mat4(1), cameraDirection) * glm::translate(glm::mat4(1), glm::vec3(0)) * glm::rotate(glm::mat4(1), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1), glm::vec3(0.01));
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, PUSH_CONSTS_SIZE, glm::value_ptr(UIMVP));
			vkCmdDraw(commandBuffer(), core.axis->vertexStream.size(), 1, 0, 0);
			vkCmdEndRendering(commandBuffer());

			vkCmdBeginRendering(commandBuffer(), &core.geometryPipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, core.geometryPipeline->pipeline);
			core.scene->drawScene(commandBuffer(), core.geometryPipeline, MPipeline::universalPipelineLayout);
			vkCmdEndRendering(commandBuffer());
			submit(&imageAvailableSemaphores);
		}
	};

	class CacheViewerPass final : public IRenderPass
	{
	public:
		CacheViewerPass() : IRenderPass("CacheViewer") {}

		void execute(RenderFrameContext& context) override
		{
			auto& core = context.renderCore;
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &core.cacheViewerPipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, core.cacheViewerPipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &core.cacheViewerPipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, &core.quadVertexBuffer, &offsets);
			universalPushConst pushConstants{};
			pushConstants.v4 = glm::vec4(invCameraPos, RADIANCE_CACHE_RAD);
			pushConstants.v4_2 = glm::vec4(CHUNK_SIZE);
			pushConstants.m4 = glm::mat4(1);
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &pushConstants);
			vkCmdDraw(commandBuffer(), 6, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}
	};

	class DeferredPass final : public IRenderPass
	{
	public:
		DeferredPass() : IRenderPass("Deferred") {}

		void execute(RenderFrameContext& context) override
		{
			auto& core = context.renderCore;
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &core.deferredPipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, core.deferredPipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &core.deferredPipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, &core.quadVertexBuffer, &offsets);
			universalPushConst pushConstants{};
			pushConstants.v4 = glm::vec4(INNER_WIDTH, INNER_HEIGHT, SSP, SSP_2);
			pushConstants.v4_2 = glm::vec4(RADIANCE_CACHE_RAD, CHUNK_SIZE, NEAR_PLANE, FAR_PLANE);
			pushConstants.m4 = glm::mat4(invCameraPos == historicalInvCameraPos ? 0 : 1);
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &pushConstants);
			vkCmdDraw(commandBuffer(), 6, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}
	};

	class InjectorPass final : public IRenderPass
	{
	public:
		InjectorPass() : IRenderPass("Injector") {}

		void execute(RenderFrameContext& context) override
		{
			auto& core = context.renderCore;
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &core.injectorPipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, core.injectorPipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &core.injectorPipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, &core.quadVertexBuffer, &offsets);
			universalPushConst pushConstants{};
			pushConstants.v4 = glm::vec4(invCameraPos, RADIANCE_CACHE_RAD);
			pushConstants.v4_2 = glm::vec4(CHUNK_SIZE, 0, 0, 0);
			pushConstants.m4 = glm::mat4(1);
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &pushConstants);
			vkCmdDraw(commandBuffer(), 6, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}
	};

	class PreFilterPass final : public IRenderPass
	{
	public:
		PreFilterPass() : IRenderPass("PreFilter") {}

		void execute(RenderFrameContext& context) override
		{
			auto& core = context.renderCore;
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &core.preFilterPipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, core.preFilterPipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &core.preFilterPipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, &core.quadVertexBuffer, &offsets);
			universalPushConst pushConstants{};
			pushConstants.v4 = glm::vec4(INNER_WIDTH, INNER_HEIGHT, RAD, SIG);
			pushConstants.m4 = glm::mat4(1);
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &pushConstants);
			vkCmdDraw(commandBuffer(), 6, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}
	};

	class FilterPass final : public IRenderPass
	{
	public:
		FilterPass() : IRenderPass("Filter") {}

		void execute(RenderFrameContext& context) override
		{
			auto& core = context.renderCore;
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &core.filterPipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, core.filterPipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &core.filterPipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, &core.quadVertexBuffer, &offsets);
			universalPushConst pushConstants{};
			pushConstants.v4 = glm::vec4(INNER_WIDTH, INNER_HEIGHT, RAD, SIG);
			pushConstants.m4 = glm::mat4(1);
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &pushConstants);
			vkCmdDraw(commandBuffer(), 6, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}
	};

	class ForwardPass final : public IRenderPass
	{
	public:
		ForwardPass() : IRenderPass("Forward") {}

		void execute(RenderFrameContext& context) override
		{
			auto& core = context.renderCore;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &core.waterLayerPipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, core.waterLayerPipeline->pipeline);
			core.scene->drawForward(commandBuffer(), core.waterLayerPipeline, MPipeline::universalPipelineLayout);
			vkCmdEndRendering(commandBuffer());
			submit();
		}
	};

	class TaauPass final : public IRenderPass
	{
	public:
		TaauPass() : IRenderPass("TAAU") {}

		void execute(RenderFrameContext& context) override
		{
			auto& core = context.renderCore;
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &core.taauPipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, core.taauPipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &core.taauPipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, &core.quadVertexBuffer, &offsets);
			universalPushConst pushConstants{};
			pushConstants.v4 = glm::vec4(INNER_WIDTH, INNER_HEIGHT, currentSubPixel, 0);
			pushConstants.m4 = historicalVP;
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &pushConstants);
			vkCmdDraw(commandBuffer(), 6, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}
	};

	class AssemblePass final : public IRenderPass
	{
	public:
		AssemblePass() : IRenderPass("Assemble") {}

		void execute(RenderFrameContext& context) override
		{
			auto& core = context.renderCore;
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &core.assemblePipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, core.assemblePipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &core.assemblePipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, &core.quadVertexBuffer, &offsets);
			vkCmdDraw(commandBuffer(), 6, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}
	};

	class EasuPass final : public IRenderPass
	{
	public:
		EasuPass() : IRenderPass("EASU") {}

		void execute(RenderFrameContext& context) override
		{
			auto& core = context.renderCore;
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &core.easuPipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, core.easuPipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &core.easuPipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, &core.quadVertexBuffer, &offsets);
			universalPushConst pushConstants{};
			pushConstants.v4 = glm::vec4(float(INNER_WIDTH) * 2 / float(OUTER_WIDTH), float(INNER_HEIGHT) * 2 / float(OUTER_HEIGHT), 0, 0);
			pushConstants.m4 = glm::mat4(1);
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &pushConstants);
			vkCmdDraw(commandBuffer(), 6, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}
	};

	class RcasPass final : public IRenderPass
	{
	public:
		RcasPass() : IRenderPass("RCAS") {}

		void execute(RenderFrameContext& context) override
		{
			auto& core = context.renderCore;
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &core.rcasPipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, core.rcasPipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &core.rcasPipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, &core.quadVertexBuffer, &offsets);
			universalPushConst pushConstants{};
			pushConstants.v4 = glm::vec4(SHARPNESS, 0, 0, 0);
			pushConstants.m4 = glm::mat4(1);
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &pushConstants);
			vkCmdDraw(commandBuffer(), 6, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}
	};

	class InterfacePrePass final : public IRenderPass
	{
	public:
		InterfacePrePass() : IRenderPass("InterfacePre") {}

		void execute(RenderFrameContext& context) override
		{
			auto& core = context.renderCore;
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &core.interfacePrePipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, core.interfacePrePipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &core.interfacePrePipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, &core.p_interface->interfaceVertexBuffer, &offsets);
			universalPushConst pushConstants{};
			pushConstants.v4 = glm::vec4(OUTER_WIDTH, OUTER_HEIGHT, 20, core.p_interface->page);
			pushConstants.m4 = glm::mat4(1);
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &pushConstants);
			vkCmdDraw(commandBuffer(), core.p_interface->interfaceVertexStream.size() / 11, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}
	};

	class InterfacePass final : public IRenderPass
	{
	public:
		InterfacePass() : IRenderPass("Interface") {}

		void execute(RenderFrameContext& context) override
		{
			auto& core = context.renderCore;
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &core.interfacePipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, core.interfacePipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &core.interfacePipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, &core.quadVertexBuffer, &offsets);
			universalPushConst pushConstants{};
			pushConstants.v4 = glm::vec4(OUTER_WIDTH, OUTER_HEIGHT, 20, core.p_interface->page);
			pushConstants.m4 = glm::mat4(1);
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &pushConstants);
			vkCmdDraw(commandBuffer(), 6, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}
	};

	class FontPass final : public IRenderPass
	{
	public:
		FontPass() : IRenderPass("Font") {}

		void execute(RenderFrameContext& context) override
		{
			auto& core = context.renderCore;
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &core.fontPipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, core.fontPipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &core.fontPipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, &core.p_interface->textVertexBuffer, &offsets);
			std::vector<float> pushConstants;
			pushConstants.push_back(MInterface::page);
			pushConstants.insert(pushConstants.end(), MInterface::textDisableTable.begin(), MInterface::textDisableTable.end());
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, PUSH_CONSTS_SIZE, pushConstants.data());
			vkCmdDraw(commandBuffer(), core.p_interface->textVertexStream.size() / 11, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}
	};

	class Frame0Pass final : public IRenderPass
	{
	public:
		Frame0Pass() : IRenderPass("Frame0") {}

		void execute(RenderFrameContext& context) override
		{
			auto& core = context.renderCore;
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			core.frame0Pipeline->updateAttachments(swapChainImageViews[context.swapchainImageIndex]);
			vkCmdBeginRendering(commandBuffer(), &core.frame0Pipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, core.frame0Pipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &core.frame0Pipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, &core.quadVertexBuffer, &offsets);
			universalPushConst pushConstants{};
			pushConstants.v4 = glm::vec4(displayID, UIEnable, core.frame0Pipeline->image2DViews.size(), 0);
			pushConstants.m4 = glm::mat4(1);
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &pushConstants);
			vkCmdDraw(commandBuffer(), 6, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			transitionImageLayout(swapChainImages[context.swapchainImageIndex], 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
			submit();
		}
	};
}

std::vector<std::unique_ptr<IRenderPass>> createDefaultRenderPasses()
{
	std::vector<std::unique_ptr<IRenderPass>> passes;
	passes.reserve(15);
	passes.emplace_back(std::make_unique<GeometryPass>());
	passes.emplace_back(std::make_unique<CacheViewerPass>());
	passes.emplace_back(std::make_unique<DeferredPass>());
	passes.emplace_back(std::make_unique<InjectorPass>());
	passes.emplace_back(std::make_unique<PreFilterPass>());
	passes.emplace_back(std::make_unique<FilterPass>());
	passes.emplace_back(std::make_unique<ForwardPass>());
	passes.emplace_back(std::make_unique<TaauPass>());
	passes.emplace_back(std::make_unique<AssemblePass>());
	passes.emplace_back(std::make_unique<EasuPass>());
	passes.emplace_back(std::make_unique<RcasPass>());
	passes.emplace_back(std::make_unique<InterfacePrePass>());
	passes.emplace_back(std::make_unique<InterfacePass>());
	passes.emplace_back(std::make_unique<FontPass>());
	passes.emplace_back(std::make_unique<Frame0Pass>());
	return passes;
}
