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
	vector<VkImage> colorAttachmentImages;
	vector<VkImageView> colorAttachmentViews;
	vector<VkDeviceMemory> colorAttachmentMemories;
	VkImage depthImage;
	VkImageView depthView = nullptr;
	VkDeviceMemory depthMemory;
	VkSampler attachmentSampler;
	vector<VkRenderingAttachmentInfo> colorAttachmentImageInfos;
	VkRenderingAttachmentInfo depthAttachmentInfo;
	vector<VkDescriptorImageInfo> image2DInfos;
	vector<VkDescriptorImageInfo> imageCubeInfos;
	VkSampler linearSampler;
	VkSampler nearstSampler;
	MPipeline(string m_vertPath, string m_fragPath);
	MPipeline(string m_vertPath, string m_fragPath, int m_colorAttachmentCount);
	MPipeline(string m_vertPath, string m_geomPath, string m_fragPath, int m_colorAttachmentCount);
	MPipeline(int padding, string m_taskPath, string m_meshPath, string m_fragPath, int m_colorAttachmentCount);
	void updateAttachments(VkImageView curSwapchainView);

	//TODO
	MPipelineType pipelineType;
	int colorAttachmentCount;
	VkFormat colorAttachmentFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	string vertPath = "";
	string fragPath = "";
	string geomPath = "";
	string taskPath = "";
	string meshPath = "";
	vector<VkImageView> image2DViews;
	vector<VkImageView> imageCubeViews;
	VkImageView indirectCacheView_1;
	VkImageView indirectCacheView_2;
	VkBuffer* pVertexBuffer = nullptr;
	VkBuffer* pStorageBuffer = nullptr;
	vector<VkFormat> colorAttachmentFormats;
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

