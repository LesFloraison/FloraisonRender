#pragma once
#include "encapVk.h"
#include <vector>
#include <stdexcept>
#include <iostream>
#include <fstream>
class MPipeline
{
public:
	static VkDescriptorPool universalDescriptorPool;
	static VkPipelineLayout universalPipelineLayout;
	static VkDescriptorSetLayout universalDescriptorSetLayout;
	enum MPipelineType {M_PIPELINE_GENERAL, M_PIPELINE_FRAME0};
	VkPipeline pipeline;
	VkBuffer uniformBuffer;
	VkDeviceMemory uniformMemory;
	void* uniformBuffersMapped;
	VkDescriptorSet descriptorSets;
	VkRenderingInfo renderingInfo;
	std::vector<VkImage> colorAttachmentImages;
	std::vector<VkImageView> colorAttachmentViews;
	std::vector<VkDeviceMemory> colorAttachmentMemories;
	VkImage depthImage;
	VkImageView depthView = nullptr;
	VkDeviceMemory depthMemory;
	VkSampler attachmentSampler;
	std::vector<VkRenderingAttachmentInfo> colorAttachmentImageInfos;
	VkRenderingAttachmentInfo depthAttachmentInfo;
	std::vector<VkDescriptorImageInfo> image2DInfos;
	std::vector<VkDescriptorImageInfo> imageCubeInfos;
	VkSampler linearSampler;
	VkSampler nearstSampler;
	MPipeline(std::string m_vertPath, std::string m_fragPath);
	MPipeline(std::string m_vertPath, std::string m_fragPath, int m_colorAttachmentCount);
	MPipeline(std::string m_vertPath, std::string m_geomPath, std::string m_fragPath, int m_colorAttachmentCount);
	MPipeline(int padding, std::string m_taskPath, std::string m_meshPath, std::string m_fragPath, int m_colorAttachmentCount);
	void updateAttachments(VkImageView curSwapchainView);

	//TODO
	MPipelineType pipelineType;
	int colorAttachmentCount;
	VkFormat colorAttachmentFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	std::string vertPath = "";
	std::string fragPath = "";
	std::string geomPath = "";
	std::string taskPath = "";
	std::string meshPath = "";
	std::vector<VkImageView> image2DViews;
	std::vector<VkImageView> imageCubeViews;
	VkImageView indirectCacheView_1;
	VkImageView indirectCacheView_2;
	VkBuffer* pVertexBuffer = nullptr;
	VkBuffer* pStorageBuffer = nullptr;
	std::vector<VkFormat> colorAttachmentFormats;
	VkBuffer* TLASBuffer;
	VkAccelerationStructureKHR* TLAS;
	unsigned int pipelineWidth = INNER_WIDTH;
	unsigned int pipelineHeight = INNER_HEIGHT;
	VkAttachmentLoadOp depthAttachmentLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	//VkBuffer* pTLASBuffer = nullptr;
	//VkAccelerationStructureKHR* pTLAS = nullptr;
	void createPipeline();
private:
	void createSampler();
	void createUniformBuffer();
	void createDescriptorSets();
	void createColorAttachments();
	void createGraphicsPipeline();
};

