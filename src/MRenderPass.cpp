#include "MRenderPass.h"

#include "MInterface.h"
#include "MPipeline.h"
#include "MScene.h"
#include "objLoader.h"

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
	// Keep the existing synchronization behavior during this refactor.
	vkQueueWaitIdle(graphicsPresentQueue);
	beginRecord(&passCommandBuffer);
}

void IRenderPass::submit(VkSemaphore* waitSemaphore, VkSemaphore* signalSemaphore)
{
	endRecordSubmit(&passCommandBuffer, waitSemaphore, signalSemaphore);
}

namespace
{
	RenderTexture getColorOutput(const MPipeline& pipeline, size_t index)
	{
		return { pipeline.colorAttachmentImages[index], pipeline.colorAttachmentViews[index] };
	}

	void configureUniversalSceneBindings(MPipeline& pipeline, RenderSetupContext& context)
	{
		// MPipeline currently has a universal descriptor layout, so every pass still
		// needs valid scene bindings until descriptor layouts are split per pass.
		pipeline.TLAS = &context.scene.TLAS;
		pipeline.TLASBuffer = &context.scene.TLASBuffer;
	}

	template<typename T>
	T& addPass(
		std::vector<std::unique_ptr<IRenderPass>>& passes,
		RenderSetupContext& context,
		typename T::Inputs inputs)
	{
		auto pass = std::make_unique<T>(std::move(inputs));
		pass->setup(context);
		T& result = *pass;
		passes.emplace_back(std::move(pass));
		return result;
	}

	class GeometryPass final : public IRenderPass
	{
	public:
		struct Inputs
		{
			MScene* scene;
			objLoader* axis;
			VkBuffer* cubeVertexBuffer;
			const std::vector<VkImageView>* materialTextures;
			const std::vector<VkImageView>* cubemaps;
		};

		struct Outputs
		{
			RenderTexture position;
			RenderTexture normal;
			RenderTexture albedo;
			VkImageView depth = VK_NULL_HANDLE;
			RenderTexture sky;
			RenderTexture uiLayer;
		};

		explicit GeometryPass(Inputs inputs)
			: IRenderPass("Geometry"), inputs(std::move(inputs)) {}

		void setup(RenderSetupContext& context) override
		{
			geometryPipeline = std::make_unique<MPipeline>(0, "shaders/geometry.task", "shaders/geometry.mesh", "shaders/geometry.frag", 3);
			geometryPipeline->colorAttachmentFormats = {
				VK_FORMAT_R32G32B32A32_SFLOAT,
				VK_FORMAT_R16G16B16A16_SFLOAT,
				VK_FORMAT_R16G16B16A16_SFLOAT
			};
			geometryPipeline->pVertexBuffer = &objLoader::objReferenceBuffer;
			geometryPipeline->pStorageBuffer = &inputs.scene->sceneInstanceBuffer;
			geometryPipeline->image2DViews = *inputs.materialTextures;
			configureUniversalSceneBindings(*geometryPipeline, context);
			geometryPipeline->createPipeline();

			uiLayerPipeline = std::make_unique<MPipeline>("shaders/UI.vert", "shaders/UI.frag", 1);
			uiLayerPipeline->image2DViews = *inputs.materialTextures;
			configureUniversalSceneBindings(*uiLayerPipeline, context);
			uiLayerPipeline->createPipeline();

			skyboxPipeline = std::make_unique<MPipeline>("shaders/skyboxSampler.vert", "shaders/skyboxSampler.frag", 1);
			skyboxPipeline->image2DViews = *inputs.materialTextures;
			skyboxPipeline->imageCubeViews = *inputs.cubemaps;
			configureUniversalSceneBindings(*skyboxPipeline, context);
			skyboxPipeline->createPipeline();

			passOutputs.position = getColorOutput(*geometryPipeline, 0);
			passOutputs.normal = getColorOutput(*geometryPipeline, 1);
			passOutputs.albedo = getColorOutput(*geometryPipeline, 2);
			passOutputs.depth = geometryPipeline->depthView;
			passOutputs.sky = getColorOutput(*skyboxPipeline, 0);
			passOutputs.uiLayer = getColorOutput(*uiLayerPipeline, 0);
		}

		void execute(RenderFrameContext& context) override
		{
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();

			vkCmdBeginRendering(commandBuffer(), &skyboxPipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &skyboxPipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, inputs.cubeVertexBuffer, &offsets);
			glm::mat4 P_lookat = proj * lookat;
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, PUSH_CONSTS_SIZE, glm::value_ptr(P_lookat));
			vkCmdDraw(commandBuffer(), 36, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());

			vkCmdBeginRendering(commandBuffer(), &uiLayerPipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, uiLayerPipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &uiLayerPipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, &inputs.axis->vertexBuffer, &offsets);
			glm::mat4 UIMVP = proj * lookat * glm::translate(glm::mat4(1), cameraDirection) * glm::translate(glm::mat4(1), glm::vec3(0)) * glm::rotate(glm::mat4(1), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1), glm::vec3(0.01));
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, PUSH_CONSTS_SIZE, glm::value_ptr(UIMVP));
			vkCmdDraw(commandBuffer(), inputs.axis->vertexStream.size(), 1, 0, 0);
			vkCmdEndRendering(commandBuffer());

			vkCmdBeginRendering(commandBuffer(), &geometryPipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, geometryPipeline->pipeline);
			inputs.scene->drawScene(commandBuffer(), geometryPipeline.get(), MPipeline::universalPipelineLayout);
			vkCmdEndRendering(commandBuffer());
			submit(&imageAvailableSemaphores);
		}

		const Outputs& outputs() const { return passOutputs; }

	private:
		Inputs inputs;
		Outputs passOutputs;
		std::unique_ptr<MPipeline> geometryPipeline;
		std::unique_ptr<MPipeline> skyboxPipeline;
		std::unique_ptr<MPipeline> uiLayerPipeline;
	};

	class CacheViewerPass final : public IRenderPass
	{
	public:
		struct Inputs
		{
			GeometryPass::Outputs geometry;
			VkBuffer* quadVertexBuffer;
			RadianceCacheResources radianceCache;
		};

		struct Outputs
		{
			RenderTexture first;
			RenderTexture second;
		};

		explicit CacheViewerPass(Inputs inputs)
			: IRenderPass("CacheViewer"), inputs(std::move(inputs)) {}

		void setup(RenderSetupContext& context) override
		{
			pipeline = std::make_unique<MPipeline>("shaders/screen.vert", "shaders/cacheViewer.frag", 2);
			pipeline->image2DViews = { inputs.geometry.position.view, inputs.geometry.normal.view };
			pipeline->indirectCacheView_1 = inputs.radianceCache.first;
			pipeline->indirectCacheView_2 = inputs.radianceCache.second;
			configureUniversalSceneBindings(*pipeline, context);
			pipeline->createPipeline();
			passOutputs.first = getColorOutput(*pipeline, 0);
			passOutputs.second = getColorOutput(*pipeline, 1);
		}

		void execute(RenderFrameContext& context) override
		{
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &pipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &pipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, inputs.quadVertexBuffer, &offsets);
			universalPushConst pushConstants{};
			pushConstants.v4 = glm::vec4(invCameraPos, RADIANCE_CACHE_RAD);
			pushConstants.v4_2 = glm::vec4(CHUNK_SIZE);
			pushConstants.m4 = glm::mat4(1);
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &pushConstants);
			vkCmdDraw(commandBuffer(), 6, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}

		const Outputs& outputs() const { return passOutputs; }

	private:
		Inputs inputs;
		Outputs passOutputs;
		std::unique_ptr<MPipeline> pipeline;
	};

	class DeferredPass final : public IRenderPass
	{
	public:
		struct Inputs
		{
			GeometryPass::Outputs geometry;
			CacheViewerPass::Outputs cacheViewer;
			TemporalHistoryResources history;
			RadianceCacheResources radianceCache;
			MScene* scene;
			VkBuffer* sampleStorageBuffer;
			VkBuffer* quadVertexBuffer;
			const std::vector<VkImageView>* materialTextures;
			const std::vector<VkImageView>* cubemaps;
		};

		struct Outputs
		{
			RenderTexture direct;
			RenderTexture indirect;
		};

		explicit DeferredPass(Inputs inputs)
			: IRenderPass("Deferred"), inputs(std::move(inputs)) {}

		void setup(RenderSetupContext& context) override
		{
			std::vector<VkImageView> resources = {
				inputs.geometry.position.view,
				inputs.geometry.normal.view,
				inputs.geometry.albedo.view,
				inputs.history.taauPosition,
				inputs.history.taauNormal,
				inputs.history.direct,
				inputs.history.indirect,
				inputs.cacheViewer.second.view
			};
			resources.insert(resources.end(), inputs.materialTextures->begin(), inputs.materialTextures->end());

			pipeline = std::make_unique<MPipeline>("shaders/screen.vert", "shaders/deferred.frag", 2);
			pipeline->image2DViews = std::move(resources);
			pipeline->pVertexBuffer = &inputs.scene->sceneVertexBuffer;
			pipeline->pStorageBuffer = inputs.sampleStorageBuffer;
			pipeline->imageCubeViews = *inputs.cubemaps;
			pipeline->indirectCacheView_1 = inputs.radianceCache.first;
			pipeline->indirectCacheView_2 = inputs.radianceCache.second;
			configureUniversalSceneBindings(*pipeline, context);
			pipeline->createPipeline();

			passOutputs.direct = getColorOutput(*pipeline, 0);
			passOutputs.indirect = getColorOutput(*pipeline, 1);
			transitionImageLayout(passOutputs.direct.image, 1, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			transitionImageLayout(passOutputs.indirect.image, 1, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		}

		void execute(RenderFrameContext& context) override
		{
			if (context.sceneUniform != nullptr) {
				updateUniformBuffer(pipeline->uniformBuffersMapped, context.sceneUniform);
			}
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &pipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &pipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, inputs.quadVertexBuffer, &offsets);
			universalPushConst pushConstants{};
			pushConstants.v4 = glm::vec4(INNER_WIDTH, INNER_HEIGHT, SSP, SSP_2);
			pushConstants.v4_2 = glm::vec4(RADIANCE_CACHE_RAD, CHUNK_SIZE, NEAR_PLANE, FAR_PLANE);
			pushConstants.m4 = glm::mat4(invCameraPos == historicalInvCameraPos ? 0 : 1);
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &pushConstants);
			vkCmdDraw(commandBuffer(), 6, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}

		const Outputs& outputs() const { return passOutputs; }

	private:
		Inputs inputs;
		Outputs passOutputs;
		std::unique_ptr<MPipeline> pipeline;
	};

	class InjectorPass final : public IRenderPass
	{
	public:
		struct Inputs
		{
			GeometryPass::Outputs geometry;
			DeferredPass::Outputs deferred;
			RadianceCacheResources radianceCache;
			VkBuffer* quadVertexBuffer;
		};

		struct Outputs
		{
			RenderTexture marker;
		};

		explicit InjectorPass(Inputs inputs)
			: IRenderPass("Injector"), inputs(std::move(inputs)) {}

		void setup(RenderSetupContext& context) override
		{
			pipeline = std::make_unique<MPipeline>("shaders/screen.vert", "shaders/injector.frag", 1);
			pipeline->image2DViews = {
				inputs.geometry.position.view,
				inputs.geometry.normal.view,
				inputs.deferred.indirect.view
			};
			pipeline->indirectCacheView_1 = inputs.radianceCache.first;
			pipeline->indirectCacheView_2 = inputs.radianceCache.second;
			configureUniversalSceneBindings(*pipeline, context);
			pipeline->createPipeline();
			passOutputs.marker = getColorOutput(*pipeline, 0);
		}

		void execute(RenderFrameContext& context) override
		{
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &pipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &pipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, inputs.quadVertexBuffer, &offsets);
			universalPushConst pushConstants{};
			pushConstants.v4 = glm::vec4(invCameraPos, RADIANCE_CACHE_RAD);
			pushConstants.v4_2 = glm::vec4(CHUNK_SIZE, 0, 0, 0);
			pushConstants.m4 = glm::mat4(1);
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &pushConstants);
			vkCmdDraw(commandBuffer(), 6, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}

		const Outputs& outputs() const { return passOutputs; }

	private:
		Inputs inputs;
		Outputs passOutputs;
		std::unique_ptr<MPipeline> pipeline;
	};

	class PreFilterPass final : public IRenderPass
	{
	public:
		struct Inputs
		{
			GeometryPass::Outputs geometry;
			DeferredPass::Outputs deferred;
			VkBuffer* quadVertexBuffer;
		};

		struct Outputs
		{
			RenderTexture direct;
			RenderTexture indirect;
		};

		explicit PreFilterPass(Inputs inputs)
			: IRenderPass("PreFilter"), inputs(std::move(inputs)) {}

		void setup(RenderSetupContext& context) override
		{
			pipeline = std::make_unique<MPipeline>("shaders/screen.vert", "shaders/preFilter.frag", 2);
			pipeline->image2DViews = {
				inputs.geometry.position.view,
				inputs.geometry.normal.view,
				inputs.geometry.albedo.view,
				inputs.deferred.direct.view,
				inputs.deferred.indirect.view
			};
			configureUniversalSceneBindings(*pipeline, context);
			pipeline->createPipeline();
			passOutputs.direct = getColorOutput(*pipeline, 0);
			passOutputs.indirect = getColorOutput(*pipeline, 1);
		}

		void execute(RenderFrameContext& context) override
		{
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &pipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &pipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, inputs.quadVertexBuffer, &offsets);
			universalPushConst pushConstants{};
			pushConstants.v4 = glm::vec4(INNER_WIDTH, INNER_HEIGHT, RAD, SIG);
			pushConstants.m4 = glm::mat4(1);
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &pushConstants);
			vkCmdDraw(commandBuffer(), 6, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}

		const Outputs& outputs() const { return passOutputs; }

	private:
		Inputs inputs;
		Outputs passOutputs;
		std::unique_ptr<MPipeline> pipeline;
	};

	class FilterPass final : public IRenderPass
	{
	public:
		struct Inputs
		{
			GeometryPass::Outputs geometry;
			PreFilterPass::Outputs preFilter;
			VkBuffer* quadVertexBuffer;
		};

		struct Outputs
		{
			RenderTexture filtered;
		};

		explicit FilterPass(Inputs inputs)
			: IRenderPass("Filter"), inputs(std::move(inputs)) {}

		void setup(RenderSetupContext& context) override
		{
			pipeline = std::make_unique<MPipeline>("shaders/screen.vert", "shaders/filter.frag", 1);
			pipeline->image2DViews = {
				inputs.geometry.position.view,
				inputs.geometry.normal.view,
				inputs.geometry.albedo.view,
				inputs.preFilter.direct.view,
				inputs.preFilter.indirect.view
			};
			configureUniversalSceneBindings(*pipeline, context);
			pipeline->createPipeline();
			passOutputs.filtered = getColorOutput(*pipeline, 0);
		}

		void execute(RenderFrameContext& context) override
		{
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &pipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &pipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, inputs.quadVertexBuffer, &offsets);
			universalPushConst pushConstants{};
			pushConstants.v4 = glm::vec4(INNER_WIDTH, INNER_HEIGHT, RAD, SIG);
			pushConstants.m4 = glm::mat4(1);
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &pushConstants);
			vkCmdDraw(commandBuffer(), 6, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}

		const Outputs& outputs() const { return passOutputs; }

	private:
		Inputs inputs;
		Outputs passOutputs;
		std::unique_ptr<MPipeline> pipeline;
	};

	class ForwardPass final : public IRenderPass
	{
	public:
		struct Inputs
		{
			GeometryPass::Outputs geometry;
			FilterPass::Outputs filter;
			MScene* scene;
			VkBuffer* sampleStorageBuffer;
			const std::vector<VkImageView>* materialTextures;
			const std::vector<VkImageView>* cubemaps;
		};

		struct Outputs
		{
			RenderTexture color;
		};

		explicit ForwardPass(Inputs inputs)
			: IRenderPass("Forward"), inputs(std::move(inputs)) {}

		void setup(RenderSetupContext& context) override
		{
			pipeline = std::make_unique<MPipeline>("shaders/waterLayer.vert", "shaders/waterLayer.frag", 1);
			pipeline->image2DViews[0] = inputs.geometry.position.view;
			pipeline->image2DViews.push_back(inputs.filter.filtered.view);
			pipeline->image2DViews.insert(pipeline->image2DViews.end(), inputs.materialTextures->begin(), inputs.materialTextures->end());
			pipeline->depthView = inputs.geometry.depth;
			pipeline->depthAttachmentLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			pipeline->pVertexBuffer = &inputs.scene->sceneVertexBuffer;
			pipeline->pStorageBuffer = inputs.sampleStorageBuffer;
			pipeline->imageCubeViews = *inputs.cubemaps;
			configureUniversalSceneBindings(*pipeline, context);
			pipeline->createPipeline();
			passOutputs.color = getColorOutput(*pipeline, 0);
		}

		void execute(RenderFrameContext& context) override
		{
			if (context.sceneUniform != nullptr) {
				updateUniformBuffer(pipeline->uniformBuffersMapped, context.sceneUniform);
			}
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &pipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
			inputs.scene->drawForward(commandBuffer(), pipeline.get(), MPipeline::universalPipelineLayout);
			vkCmdEndRendering(commandBuffer());
			submit();
		}

		const Outputs& outputs() const { return passOutputs; }

	private:
		Inputs inputs;
		Outputs passOutputs;
		std::unique_ptr<MPipeline> pipeline;
	};

	class TaauPass final : public IRenderPass
	{
	public:
		struct Inputs
		{
			GeometryPass::Outputs geometry;
			FilterPass::Outputs filter;
			TemporalHistoryResources history;
			VkBuffer* quadVertexBuffer;
		};

		struct Outputs
		{
			RenderTexture color;
			RenderTexture position;
			RenderTexture normal;
		};

		explicit TaauPass(Inputs inputs)
			: IRenderPass("TAAU"), inputs(std::move(inputs)) {}

		void setup(RenderSetupContext& context) override
		{
			pipeline = std::make_unique<MPipeline>("shaders/screen.vert", "shaders/taau.frag", 3);
			pipeline->colorAttachmentFormats = {
				VK_FORMAT_R16G16B16A16_SFLOAT,
				VK_FORMAT_R32G32B32A32_SFLOAT,
				VK_FORMAT_R16G16B16A16_SFLOAT
			};
			pipeline->image2DViews = {
				inputs.geometry.position.view,
				inputs.geometry.normal.view,
				inputs.filter.filtered.view,
				inputs.history.taau,
				inputs.history.taauPosition,
				inputs.history.taauNormal
			};
			pipeline->pipelineWidth = INNER_WIDTH * 2;
			pipeline->pipelineHeight = INNER_HEIGHT * 2;
			configureUniversalSceneBindings(*pipeline, context);
			pipeline->createPipeline();

			passOutputs.color = getColorOutput(*pipeline, 0);
			passOutputs.position = getColorOutput(*pipeline, 1);
			passOutputs.normal = getColorOutput(*pipeline, 2);
			transitionImageLayout(passOutputs.color.image, 1, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			transitionImageLayout(passOutputs.position.image, 1, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			transitionImageLayout(passOutputs.normal.image, 1, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		}

		void execute(RenderFrameContext& context) override
		{
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &pipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &pipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, inputs.quadVertexBuffer, &offsets);
			universalPushConst pushConstants{};
			pushConstants.v4 = glm::vec4(INNER_WIDTH, INNER_HEIGHT, currentSubPixel, 0);
			pushConstants.m4 = historicalVP;
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &pushConstants);
			vkCmdDraw(commandBuffer(), 6, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}

		const Outputs& outputs() const { return passOutputs; }

	private:
		Inputs inputs;
		Outputs passOutputs;
		std::unique_ptr<MPipeline> pipeline;
	};

	class AssemblePass final : public IRenderPass
	{
	public:
		struct Inputs
		{
			TaauPass::Outputs taau;
			RenderTexture sky;
			ForwardPass::Outputs forward;
			VkBuffer* quadVertexBuffer;
		};

		struct Outputs
		{
			RenderTexture color;
		};

		explicit AssemblePass(Inputs inputs)
			: IRenderPass("Assemble"), inputs(std::move(inputs)) {}

		void setup(RenderSetupContext& context) override
		{
			pipeline = std::make_unique<MPipeline>("shaders/screen.vert", "shaders/assemble.frag", 1);
			pipeline->image2DViews = { inputs.taau.color.view, inputs.sky.view, inputs.forward.color.view };
			pipeline->pipelineWidth = INNER_WIDTH * 2;
			pipeline->pipelineHeight = INNER_HEIGHT * 2;
			configureUniversalSceneBindings(*pipeline, context);
			pipeline->createPipeline();
			passOutputs.color = getColorOutput(*pipeline, 0);
		}

		void execute(RenderFrameContext& context) override
		{
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &pipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &pipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, inputs.quadVertexBuffer, &offsets);
			vkCmdDraw(commandBuffer(), 6, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}

		const Outputs& outputs() const { return passOutputs; }

	private:
		Inputs inputs;
		Outputs passOutputs;
		std::unique_ptr<MPipeline> pipeline;
	};

	class EasuPass final : public IRenderPass
	{
	public:
		struct Inputs
		{
			AssemblePass::Outputs assemble;
			VkBuffer* quadVertexBuffer;
		};

		struct Outputs
		{
			RenderTexture color;
		};

		explicit EasuPass(Inputs inputs)
			: IRenderPass("EASU"), inputs(std::move(inputs)) {}

		void setup(RenderSetupContext& context) override
		{
			pipeline = std::make_unique<MPipeline>("shaders/screen.vert", "shaders/easu.frag", 1);
			pipeline->image2DViews = { inputs.assemble.color.view };
			pipeline->pipelineWidth = OUTER_WIDTH;
			pipeline->pipelineHeight = OUTER_HEIGHT;
			configureUniversalSceneBindings(*pipeline, context);
			pipeline->createPipeline();
			passOutputs.color = getColorOutput(*pipeline, 0);
		}

		void execute(RenderFrameContext& context) override
		{
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &pipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &pipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, inputs.quadVertexBuffer, &offsets);
			universalPushConst pushConstants{};
			pushConstants.v4 = glm::vec4(float(INNER_WIDTH) * 2 / float(OUTER_WIDTH), float(INNER_HEIGHT) * 2 / float(OUTER_HEIGHT), 0, 0);
			pushConstants.m4 = glm::mat4(1);
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &pushConstants);
			vkCmdDraw(commandBuffer(), 6, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}

		const Outputs& outputs() const { return passOutputs; }

	private:
		Inputs inputs;
		Outputs passOutputs;
		std::unique_ptr<MPipeline> pipeline;
	};

	class RcasPass final : public IRenderPass
	{
	public:
		struct Inputs
		{
			EasuPass::Outputs easu;
			VkBuffer* quadVertexBuffer;
		};

		struct Outputs
		{
			RenderTexture color;
		};

		explicit RcasPass(Inputs inputs)
			: IRenderPass("RCAS"), inputs(std::move(inputs)) {}

		void setup(RenderSetupContext& context) override
		{
			pipeline = std::make_unique<MPipeline>("shaders/screen.vert", "shaders/rcas.frag", 1);
			pipeline->image2DViews = { inputs.easu.color.view };
			pipeline->pipelineWidth = OUTER_WIDTH;
			pipeline->pipelineHeight = OUTER_HEIGHT;
			configureUniversalSceneBindings(*pipeline, context);
			pipeline->createPipeline();
			passOutputs.color = getColorOutput(*pipeline, 0);
		}

		void execute(RenderFrameContext& context) override
		{
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &pipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &pipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, inputs.quadVertexBuffer, &offsets);
			universalPushConst pushConstants{};
			pushConstants.v4 = glm::vec4(SHARPNESS, 0, 0, 0);
			pushConstants.m4 = glm::mat4(1);
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &pushConstants);
			vkCmdDraw(commandBuffer(), 6, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}

		const Outputs& outputs() const { return passOutputs; }

	private:
		Inputs inputs;
		Outputs passOutputs;
		std::unique_ptr<MPipeline> pipeline;
	};

	class InterfacePrePass final : public IRenderPass
	{
	public:
		struct Inputs
		{
			RcasPass::Outputs rcas;
			MInterface* interfaceLayer;
			const std::vector<VkImageView>* interfaceTextures;
		};

		struct Outputs
		{
			RenderTexture background;
			RenderTexture interfaceTexture;
		};

		explicit InterfacePrePass(Inputs inputs)
			: IRenderPass("InterfacePre"), inputs(std::move(inputs)) {}

		void setup(RenderSetupContext& context) override
		{
			std::vector<VkImageView> resources = { inputs.rcas.color.view };
			resources.insert(resources.end(), inputs.interfaceTextures->begin(), inputs.interfaceTextures->end());
			pipeline = std::make_unique<MPipeline>("shaders/interfacePre.vert", "shaders/interfacePre.frag", 2);
			pipeline->image2DViews = std::move(resources);
			pipeline->pipelineWidth = OUTER_WIDTH;
			pipeline->pipelineHeight = OUTER_HEIGHT;
			configureUniversalSceneBindings(*pipeline, context);
			pipeline->createPipeline();
			passOutputs.background = getColorOutput(*pipeline, 0);
			passOutputs.interfaceTexture = getColorOutput(*pipeline, 1);
		}

		void execute(RenderFrameContext& context) override
		{
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &pipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &pipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, &inputs.interfaceLayer->interfaceVertexBuffer, &offsets);
			universalPushConst pushConstants{};
			pushConstants.v4 = glm::vec4(OUTER_WIDTH, OUTER_HEIGHT, 20, inputs.interfaceLayer->page);
			pushConstants.m4 = glm::mat4(1);
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &pushConstants);
			vkCmdDraw(commandBuffer(), inputs.interfaceLayer->interfaceVertexStream.size() / 11, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}

		const Outputs& outputs() const { return passOutputs; }

	private:
		Inputs inputs;
		Outputs passOutputs;
		std::unique_ptr<MPipeline> pipeline;
	};

	class InterfacePass final : public IRenderPass
	{
	public:
		struct Inputs
		{
			InterfacePrePass::Outputs interfacePre;
			MInterface* interfaceLayer;
			VkBuffer* quadVertexBuffer;
		};

		struct Outputs
		{
			RenderTexture color;
		};

		explicit InterfacePass(Inputs inputs)
			: IRenderPass("Interface"), inputs(std::move(inputs)) {}

		void setup(RenderSetupContext& context) override
		{
			pipeline = std::make_unique<MPipeline>("shaders/screen.vert", "shaders/interface.frag", 1);
			pipeline->image2DViews = {
				inputs.interfacePre.background.view,
				inputs.interfacePre.interfaceTexture.view
			};
			pipeline->pipelineWidth = OUTER_WIDTH;
			pipeline->pipelineHeight = OUTER_HEIGHT;
			configureUniversalSceneBindings(*pipeline, context);
			pipeline->createPipeline();
			passOutputs.color = getColorOutput(*pipeline, 0);
		}

		void execute(RenderFrameContext& context) override
		{
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &pipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &pipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, inputs.quadVertexBuffer, &offsets);
			universalPushConst pushConstants{};
			pushConstants.v4 = glm::vec4(OUTER_WIDTH, OUTER_HEIGHT, 20, inputs.interfaceLayer->page);
			pushConstants.m4 = glm::mat4(1);
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &pushConstants);
			vkCmdDraw(commandBuffer(), 6, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}

		const Outputs& outputs() const { return passOutputs; }

	private:
		Inputs inputs;
		Outputs passOutputs;
		std::unique_ptr<MPipeline> pipeline;
	};

	class FontPass final : public IRenderPass
	{
	public:
		struct Inputs
		{
			MInterface* interfaceLayer;
			const std::vector<VkImageView>* fontTextures;
		};

		struct Outputs
		{
			RenderTexture color;
		};

		explicit FontPass(Inputs inputs)
			: IRenderPass("Font"), inputs(std::move(inputs)) {}

		void setup(RenderSetupContext& context) override
		{
			pipeline = std::make_unique<MPipeline>("shaders/font.vert", "shaders/font.frag", 1);
			pipeline->image2DViews = *inputs.fontTextures;
			pipeline->pipelineWidth = OUTER_WIDTH;
			pipeline->pipelineHeight = OUTER_HEIGHT;
			configureUniversalSceneBindings(*pipeline, context);
			pipeline->createPipeline();
			passOutputs.color = getColorOutput(*pipeline, 0);
		}

		void execute(RenderFrameContext& context) override
		{
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			vkCmdBeginRendering(commandBuffer(), &pipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &pipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, &inputs.interfaceLayer->textVertexBuffer, &offsets);
			std::vector<float> pushConstants;
			pushConstants.push_back(MInterface::page);
			pushConstants.insert(pushConstants.end(), MInterface::textDisableTable.begin(), MInterface::textDisableTable.end());
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, PUSH_CONSTS_SIZE, pushConstants.data());
			vkCmdDraw(commandBuffer(), inputs.interfaceLayer->textVertexStream.size() / 11, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			submit();
		}

		const Outputs& outputs() const { return passOutputs; }

	private:
		Inputs inputs;
		Outputs passOutputs;
		std::unique_ptr<MPipeline> pipeline;
	};

	class Frame0Pass final : public IRenderPass
	{
	public:
		struct Inputs
		{
			GeometryPass::Outputs geometry;
			CacheViewerPass::Outputs cacheViewer;
			DeferredPass::Outputs deferred;
			FilterPass::Outputs filter;
			ForwardPass::Outputs forward;
			TaauPass::Outputs taau;
			AssemblePass::Outputs assemble;
			EasuPass::Outputs easu;
			RcasPass::Outputs rcas;
			InterfacePass::Outputs interfaceOutput;
			FontPass::Outputs font;
			VkBuffer* quadVertexBuffer;
		};

		struct Outputs {};

		explicit Frame0Pass(Inputs inputs)
			: IRenderPass("Frame0"), inputs(std::move(inputs)) {}

		void setup(RenderSetupContext& context) override
		{
			std::vector<VkImageView> resources = {
				inputs.geometry.position.view,
				inputs.geometry.normal.view,
				inputs.geometry.albedo.view,
				inputs.geometry.sky.view,
				inputs.geometry.uiLayer.view,
				inputs.filter.filtered.view,
				inputs.easu.color.view,
				inputs.rcas.color.view,
				inputs.taau.color.view,
				inputs.taau.position.view,
				inputs.taau.normal.view,
				inputs.cacheViewer.first.view,
				inputs.cacheViewer.second.view,
				inputs.deferred.direct.view,
				inputs.deferred.indirect.view,
				inputs.forward.color.view,
				inputs.interfaceOutput.color.view,
				inputs.assemble.color.view,
				inputs.font.color.view
			};

			pipeline = std::make_unique<MPipeline>("shaders/screen.vert", "shaders/frame0.frag");
			pipeline->image2DViews = std::move(resources);
			pipeline->pipelineWidth = OUTER_WIDTH;
			pipeline->pipelineHeight = OUTER_HEIGHT;
			configureUniversalSceneBindings(*pipeline, context);
			pipeline->createPipeline();
		}

		void execute(RenderFrameContext& context) override
		{
			VkDeviceSize offsets = context.vertexBufferOffset;
			beginRecording();
			pipeline->updateAttachments(swapChainImageViews[context.swapchainImageIndex]);
			vkCmdBeginRendering(commandBuffer(), &pipeline->renderingInfo);
			vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
			vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, MPipeline::universalPipelineLayout, 0, 1, &pipeline->descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer(), 0, 1, inputs.quadVertexBuffer, &offsets);
			universalPushConst pushConstants{};
			pushConstants.v4 = glm::vec4(displayID, UIEnable, pipeline->image2DViews.size(), 0);
			pushConstants.m4 = glm::mat4(1);
			vkCmdPushConstants(commandBuffer(), MPipeline::universalPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(universalPushConst), &pushConstants);
			vkCmdDraw(commandBuffer(), 6, 1, 0, 0);
			vkCmdEndRendering(commandBuffer());
			transitionImageLayout(swapChainImages[context.swapchainImageIndex], 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
			submit();
		}

	private:
		Inputs inputs;
		std::unique_ptr<MPipeline> pipeline;
	};
}

RenderPassBuildResult createDefaultRenderPasses(RenderSetupContext& context)
{
	RenderPassBuildResult result;
	result.passes.reserve(15);

	auto& geometry = addPass<GeometryPass>(result.passes, context, {
		&context.scene,
		&context.axis,
		&context.cubeVertexBuffer,
		&context.materialTextures,
		&context.cubemaps
	});

	auto& cacheViewer = addPass<CacheViewerPass>(result.passes, context, {
		geometry.outputs(),
		&context.quadVertexBuffer,
		context.radianceCache
	});

	auto& deferred = addPass<DeferredPass>(result.passes, context, {
		geometry.outputs(),
		cacheViewer.outputs(),
		context.history,
		context.radianceCache,
		&context.scene,
		&context.sampleStorageBuffer,
		&context.quadVertexBuffer,
		&context.materialTextures,
		&context.cubemaps
	});

	addPass<InjectorPass>(result.passes, context, {
		geometry.outputs(),
		deferred.outputs(),
		context.radianceCache,
		&context.quadVertexBuffer
	});

	auto& preFilter = addPass<PreFilterPass>(result.passes, context, {
		geometry.outputs(),
		deferred.outputs(),
		&context.quadVertexBuffer
	});

	auto& filter = addPass<FilterPass>(result.passes, context, {
		geometry.outputs(),
		preFilter.outputs(),
		&context.quadVertexBuffer
	});

	auto& forward = addPass<ForwardPass>(result.passes, context, {
		geometry.outputs(),
		filter.outputs(),
		&context.scene,
		&context.sampleStorageBuffer,
		&context.materialTextures,
		&context.cubemaps
	});

	auto& taau = addPass<TaauPass>(result.passes, context, {
		geometry.outputs(),
		filter.outputs(),
		context.history,
		&context.quadVertexBuffer
	});

	auto& assemble = addPass<AssemblePass>(result.passes, context, {
		taau.outputs(),
		geometry.outputs().sky,
		forward.outputs(),
		&context.quadVertexBuffer
	});

	auto& easu = addPass<EasuPass>(result.passes, context, {
		assemble.outputs(),
		&context.quadVertexBuffer
	});

	auto& rcas = addPass<RcasPass>(result.passes, context, {
		easu.outputs(),
		&context.quadVertexBuffer
	});

	auto& interfacePre = addPass<InterfacePrePass>(result.passes, context, {
		rcas.outputs(),
		&context.interfaceLayer,
		&MInterface::interfaceTextureArrayViews
	});

	auto& interfacePass = addPass<InterfacePass>(result.passes, context, {
		interfacePre.outputs(),
		&context.interfaceLayer,
		&context.quadVertexBuffer
	});

	auto& font = addPass<FontPass>(result.passes, context, {
		&context.interfaceLayer,
		&MInterface::fontTextureArrayViews
	});

	addPass<Frame0Pass>(result.passes, context, {
		geometry.outputs(),
		cacheViewer.outputs(),
		deferred.outputs(),
		filter.outputs(),
		forward.outputs(),
		taau.outputs(),
		assemble.outputs(),
		easu.outputs(),
		rcas.outputs(),
		interfacePass.outputs(),
		font.outputs(),
		&context.quadVertexBuffer
	});

	result.historySources.direct = deferred.outputs().direct;
	result.historySources.indirect = deferred.outputs().indirect;
	result.historySources.taau = taau.outputs().color;
	result.historySources.taauPosition = taau.outputs().position;
	result.historySources.taauNormal = taau.outputs().normal;
	return result;
}
