#include "MPipeline.h"
#include "MRenderCore.h"

VkDescriptorPool MPipeline::universalDescriptorPool = NULL;
VkDescriptorSetLayout MPipeline::universalDescriptorSetLayout = NULL;
VkPipelineLayout MPipeline::universalPipelineLayout = NULL;

void createPipelineLayout(VkPipelineLayout* pipelineLayout, VkDescriptorSetLayout descriptorSetLayout) {
	VkPushConstantRange push_constant;
	push_constant.offset = 0;
	push_constant.size = PUSH_CONSTS_SIZE;
	push_constant.stageFlags = VK_SHADER_STAGE_ALL;


	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
	pipelineLayoutInfo.pPushConstantRanges = &push_constant; // Optional
	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}
}

static VkShaderModule createShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}
	return shaderModule;
}

static std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	return buffer;
}

void MPipeline::createUniformBuffer() {
	VkDeviceSize bufferSize = UNIFROM_BUFFER_SIZE;
	createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformMemory);
	MRenderCore::bufferPool.push_back(&uniformBuffer);
	MRenderCore::bufferMemoryPool.push_back(&uniformMemory);
	vkMapMemory(device, uniformMemory, 0, bufferSize, 0, &uniformBuffersMapped);
}

MPipeline::MPipeline(string m_vertPath, string m_fragPath)
{
	vertPath = m_vertPath;
	fragPath = m_fragPath;
	pipelineType = M_PIPELINE_FRAME0;
	colorAttachmentFormat = VK_FORMAT_B8G8R8A8_UNORM;
	colorAttachmentFormats.push_back(colorAttachmentFormat);
	colorAttachmentCount = 1;
	//image2DViews = m_image2DViews;
	//imageCubeViews = m_imageCubeViews;
	image2DViews.push_back(nullptr);
	imageCubeViews.push_back(nullptr);
	createSampler();
	createUniformBuffer();

	VkRenderingAttachmentInfo swapchainAttachment{};
	colorAttachmentImageInfos.push_back(swapchainAttachment);
}

MPipeline::MPipeline(string m_vertPath, string m_fragPath, int m_colorAttachmentCount)
{
	vertPath = m_vertPath;
	fragPath = m_fragPath;
	pipelineType = M_PIPELINE_GENERAL;
	colorAttachmentCount = m_colorAttachmentCount;
	//image2DViews = m_image2DViews;
	//imageCubeViews = m_imageCubeViews;
	image2DViews.push_back(nullptr);
	imageCubeViews.push_back(nullptr);
	createSampler();
	createUniformBuffer();
}

MPipeline::MPipeline(int padding, string m_taskPath, string m_meshPath, string m_fragPath, int m_colorAttachmentCount)
{
	taskPath = m_taskPath;
	meshPath = m_meshPath;
	fragPath = m_fragPath;
	pipelineType = M_PIPELINE_GENERAL;
	colorAttachmentCount = m_colorAttachmentCount;
	image2DViews.push_back(nullptr);
	imageCubeViews.push_back(nullptr);
	createSampler();
	createUniformBuffer();
}

MPipeline::MPipeline(string m_vertPath, string m_geomPath, string m_fragPath, int m_colorAttachmentCount)
{
	vertPath = m_vertPath;
	geomPath = m_geomPath;
	fragPath = m_fragPath;
	pipelineType = M_PIPELINE_GENERAL;
	colorAttachmentCount = m_colorAttachmentCount;
	//image2DViews = m_image2DViews;
	//imageCubeViews = m_imageCubeViews;
	image2DViews.push_back(nullptr);
	imageCubeViews.push_back(nullptr);
	createSampler();
	createUniformBuffer();
}

void MPipeline::createDescriptorSets()
{
	for (int i = 0; i < image2DViews.size(); i++) {
		VkDescriptorImageInfo imageInfo;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = image2DViews[i];
		imageInfo.sampler = linearSampler;
		image2DInfos.push_back(imageInfo);
	}

	for (int i = 0; i < imageCubeViews.size(); i++) {
		VkDescriptorImageInfo imageInfo;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = imageCubeViews[i];
		imageInfo.sampler = linearSampler;
		imageCubeInfos.push_back(imageInfo);
	}

	VkDescriptorImageInfo image3DInfo_1;
	image3DInfo_1.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	image3DInfo_1.imageView = indirectCacheView_1;
	image3DInfo_1.sampler = linearSampler;

	VkDescriptorImageInfo image3DInfo_2;
	image3DInfo_2.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	image3DInfo_2.imageView = indirectCacheView_2;
	image3DInfo_2.sampler = linearSampler;

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = universalDescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &universalDescriptorSetLayout;

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = UNIFROM_BUFFER_SIZE;

	//TODO

	VkDescriptorBufferInfo asBufferInfo{};
	asBufferInfo.buffer = *TLASBuffer;
	asBufferInfo.offset = 0;
	asBufferInfo.range = 10000;

	VkWriteDescriptorSetAccelerationStructureKHR writeDescriptorSetAccelerationStructure{};
	writeDescriptorSetAccelerationStructure.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	writeDescriptorSetAccelerationStructure.accelerationStructureCount = 1;
	writeDescriptorSetAccelerationStructure.pAccelerationStructures = TLAS;

	VkDescriptorBufferInfo vertexStorageBuffer{};
	vertexStorageBuffer.buffer = pVertexBuffer == nullptr ? nullptr : *pVertexBuffer;
	vertexStorageBuffer.offset = 0;
	vertexStorageBuffer.range = VK_WHOLE_SIZE;

	VkDescriptorBufferInfo storageBuffer{};
	storageBuffer.buffer = pStorageBuffer == nullptr ? nullptr : *pStorageBuffer;
	storageBuffer.offset = 0;
	storageBuffer.range = VK_WHOLE_SIZE;

	std::array<VkWriteDescriptorSet, 8> descriptorWrites{};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSets;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSets;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = image2DInfos.size();
	descriptorWrites[1].pImageInfo = image2DInfos.data();

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = descriptorSets;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[2].descriptorCount = imageCubeInfos.size();
	descriptorWrites[2].pImageInfo = imageCubeInfos.data();

	descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[3].dstSet = descriptorSets;
	descriptorWrites[3].dstBinding = 3;
	descriptorWrites[3].dstArrayElement = 0;
	descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	descriptorWrites[3].descriptorCount = 1;
	descriptorWrites[3].pBufferInfo = &asBufferInfo;
	descriptorWrites[3].pNext = &writeDescriptorSetAccelerationStructure;

	descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[4].dstSet = descriptorSets;
	descriptorWrites[4].dstBinding = 4;
	descriptorWrites[4].dstArrayElement = 0;
	descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[4].descriptorCount = 1;
	descriptorWrites[4].pBufferInfo = &vertexStorageBuffer;
	
	descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[5].dstSet = descriptorSets;
	descriptorWrites[5].dstBinding = 5;
	descriptorWrites[5].dstArrayElement = 0;
	descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[5].descriptorCount = 1;
	descriptorWrites[5].pBufferInfo = &storageBuffer;

	descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[6].dstSet = descriptorSets;
	descriptorWrites[6].dstBinding = 6;
	descriptorWrites[6].dstArrayElement = 0;
	descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrites[6].descriptorCount = 1;
	descriptorWrites[6].pImageInfo = &image3DInfo_1;

	descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[7].dstSet = descriptorSets;
	descriptorWrites[7].dstBinding = 7;
	descriptorWrites[7].dstArrayElement = 0;
	descriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrites[7].descriptorCount = 1;
	descriptorWrites[7].pImageInfo = &image3DInfo_2;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void MPipeline::createGraphicsPipeline()
{
	vector<VkPipelineShaderStageCreateInfo> shaderStages;
	char absPath[4096] = { 0 };
	_fullpath(absPath, "bin", 4096);
	string validatorPath = absPath + string("/glslangValidator.exe");
	VkShaderModule vertShaderModule = NULL;
	VkShaderModule geomShaderModule = NULL;
	VkShaderModule taskShaderModule = NULL;
	VkShaderModule meshShaderModule = NULL;
	if (meshPath == "") {
		string vertSpvPath = "spv" + vertPath.substr(vertPath.find_last_of('/'), vertPath.length() - vertPath.find_last_of('/') - 5) + "Vert.spv";
		string compileVertCmd = validatorPath + " -V " + vertPath + " -o " + vertSpvPath;
		system(compileVertCmd.data());
		auto vertShaderCode = readFile(vertSpvPath);
		vertShaderModule = createShaderModule(vertShaderCode);
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";
		shaderStages.push_back(vertShaderStageInfo);

		if (geomPath != "") {
			string geomSpvPath = "spv" + vertPath.substr(vertPath.find_last_of('/'), vertPath.length() - vertPath.find_last_of('/') - 5) + "Geom.spv";
			string compileGeomCmd = validatorPath + " -V " + geomPath + " -o " + geomSpvPath;
			system(compileGeomCmd.data());
			auto geomShaderCode = readFile(geomSpvPath);
			geomShaderModule = createShaderModule(geomShaderCode);
			VkPipelineShaderStageCreateInfo geomShaderStageInfo{};
			geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
			geomShaderStageInfo.module = geomShaderModule;
			geomShaderStageInfo.pName = "main";
			shaderStages.push_back(geomShaderStageInfo);
		}
	}
	else {
		string taskSpvPath = "spv" + taskPath.substr(taskPath.find_last_of('/'), taskPath.length() - taskPath.find_last_of('/') - 5) + "Task.spv";
		string compiletaskCmd = validatorPath + " -V " + taskPath + " -o " + taskSpvPath;
		system(compiletaskCmd.data());
		auto taskShaderCode = readFile(taskSpvPath);
		taskShaderModule = createShaderModule(taskShaderCode);
		VkPipelineShaderStageCreateInfo taskShaderStageInfo{};
		taskShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		taskShaderStageInfo.stage = VK_SHADER_STAGE_TASK_BIT_EXT;
		taskShaderStageInfo.module = taskShaderModule;
		taskShaderStageInfo.pName = "main";
		shaderStages.push_back(taskShaderStageInfo);

		string meshSpvPath = "spv" + meshPath.substr(meshPath.find_last_of('/'), meshPath.length() - meshPath.find_last_of('/') - 5) + "Mesh.spv";
		string compilemeshCmd = validatorPath + " -V " + meshPath + " -o " + meshSpvPath;
		system(compilemeshCmd.data());
		auto meshShaderCode = readFile(meshSpvPath);
		meshShaderModule = createShaderModule(meshShaderCode);
		VkPipelineShaderStageCreateInfo meshShaderStageInfo{};
		meshShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		meshShaderStageInfo.stage = VK_SHADER_STAGE_MESH_BIT_EXT;
		meshShaderStageInfo.module = meshShaderModule;
		meshShaderStageInfo.pName = "main";
		shaderStages.push_back(meshShaderStageInfo);
	}

	string fragSpvPath = "spv" + fragPath.substr(fragPath.find_last_of('/'), fragPath.length() - fragPath.find_last_of('/') - 5) + "Frag.spv";
	string compileFragCmd = validatorPath + " -V " + fragPath + " -o " + fragSpvPath;
	system(compileFragCmd.data());
	auto fragShaderCode = readFile(fragSpvPath);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";
	shaderStages.push_back(fragShaderStageInfo);

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;


	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)pipelineWidth;
	viewport.height = (float)pipelineHeight;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = { pipelineWidth, pipelineHeight };

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
	for (int i = 0; i < colorAttachmentCount; i++) {
		colorBlendAttachments.push_back(colorBlendAttachment);
	}

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = colorAttachmentCount;
	colorBlending.pAttachments = colorBlendAttachments.data();
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional

	VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
	pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
	pipelineRenderingCreateInfo.colorAttachmentCount = colorAttachmentCount;
	pipelineRenderingCreateInfo.pColorAttachmentFormats = colorAttachmentFormats.data();
	pipelineRenderingCreateInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = shaderStages.size();
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;

	//TODO
	pipelineInfo.layout = universalPipelineLayout;
	pipelineInfo.renderPass = nullptr;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	pipelineInfo.pNext = &pipelineRenderingCreateInfo;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	if (geomShaderModule != NULL) {
		vkDestroyShaderModule(device, geomShaderModule, nullptr);
	}
	if (taskShaderModule != NULL) {
		vkDestroyShaderModule(device, taskShaderModule, nullptr);
	}
	if (meshShaderModule != NULL) {
		vkDestroyShaderModule(device, meshShaderModule, nullptr);
	}
	if (vertShaderModule != NULL) {
		vkDestroyShaderModule(device, vertShaderModule, nullptr);
	}
	MRenderCore::pipelinePool.push_back(&pipeline);
}

void MPipeline::createColorAttachments()
{
	if (colorAttachmentFormats.empty() == true) {
		for (int i = 0; i < colorAttachmentCount; i++) {
			colorAttachmentFormats.push_back(colorAttachmentFormat);
		}
	}
	for (int i = 0; i < colorAttachmentCount; i++) {
		VkImage* colorAttachmemtImage = new VkImage;
		VkImageView* colorAttachmentView = new VkImageView;
		VkDeviceMemory* colorAttachmentMemory = new VkDeviceMemory;
		createImage(colorAttachmemtImage, colorAttachmentMemory, pipelineWidth, pipelineHeight, 1, colorAttachmentFormats[i], VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		createImageView(colorAttachmentView, *colorAttachmemtImage, 1, colorAttachmentFormats[i], VK_IMAGE_ASPECT_COLOR_BIT);
		colorAttachmentImages.push_back(*colorAttachmemtImage);
		colorAttachmentViews.push_back(*colorAttachmentView);
		colorAttachmentMemories.push_back(*colorAttachmentMemory);

		VkRenderingAttachmentInfo colorAttachmentInfo{};
		colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		colorAttachmentInfo.imageView = colorAttachmentViews[i];
		colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
		colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentInfo.clearValue = { {0.0f, 0.0f, 0.0f, 0.0f} };

		colorAttachmentImageInfos.push_back(colorAttachmentInfo);
	}

	
}
void MPipeline::createSampler() {
	VkSamplerCreateInfo samplerLinearInfo{};
	samplerLinearInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerLinearInfo.magFilter = VK_FILTER_LINEAR;
	samplerLinearInfo.minFilter = VK_FILTER_LINEAR;
	samplerLinearInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerLinearInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerLinearInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerLinearInfo.anisotropyEnable = VK_TRUE;
	samplerLinearInfo.maxAnisotropy = 16;
	samplerLinearInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerLinearInfo.unnormalizedCoordinates = VK_FALSE;
	samplerLinearInfo.compareEnable = VK_FALSE;
	samplerLinearInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerLinearInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerLinearInfo.mipLodBias = 0.0f;
	samplerLinearInfo.minLod = 0.0f;
	samplerLinearInfo.maxLod = 100.f;
	if (vkCreateSampler(device, &samplerLinearInfo, nullptr, &linearSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create linear sampler!");
	}
	MRenderCore::samplerPool.push_back(&linearSampler);

	VkSamplerCreateInfo samplerNearstInfo{};
	samplerNearstInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerNearstInfo.magFilter = VK_FILTER_NEAREST;
	samplerNearstInfo.minFilter = VK_FILTER_NEAREST;
	samplerNearstInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerNearstInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerNearstInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerNearstInfo.anisotropyEnable = VK_TRUE;
	samplerNearstInfo.maxAnisotropy = 16;
	samplerNearstInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerNearstInfo.unnormalizedCoordinates = VK_FALSE;
	samplerNearstInfo.compareEnable = VK_FALSE;
	samplerNearstInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerNearstInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerNearstInfo.mipLodBias = 0.0f;
	samplerNearstInfo.minLod = 0.0f;
	samplerNearstInfo.maxLod = 100.f;
	if (vkCreateSampler(device, &samplerNearstInfo, nullptr, &nearstSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create nearst sampler!");
	}
	MRenderCore::samplerPool.push_back(&nearstSampler);
}

void MPipeline::updateAttachments(VkImageView curSwapchainView) {
	VkRenderingAttachmentInfo colorAttachmentInfo{};
	colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	colorAttachmentInfo.imageView = curSwapchainView;
	colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
	colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentInfo.clearValue = { {0.0f, 0.0f, 0.0f, 0.0f} };

	colorAttachmentImageInfos[0] = colorAttachmentInfo;

	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
	renderingInfo.renderArea.offset = { 0, 0 };
	renderingInfo.renderArea.extent = { pipelineWidth, pipelineHeight };
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = colorAttachmentImageInfos.size();
	renderingInfo.pColorAttachments = colorAttachmentImageInfos.data();
	renderingInfo.pDepthAttachment = &depthAttachmentInfo;
}

void MPipeline::createPipeline() {
	if (pipelineType == M_PIPELINE_GENERAL) {
		createColorAttachments();
	}
	if (depthView == nullptr) {
		createImage(&depthImage, &depthMemory, pipelineWidth, pipelineHeight, 1, VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		createImageView(&depthView, depthImage, 1, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT);
	}
	else {
		cout << "happppppppe" << endl;
	}
	depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;
	depthAttachmentInfo.imageView = depthView;
	depthAttachmentInfo.loadOp = depthAttachmentLoadOp;
	depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachmentInfo.clearValue = { {1.0f, 1.0f, 1.0f, 1.0f} };

	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
	renderingInfo.renderArea.offset = { 0, 0 };
	renderingInfo.renderArea.extent = { pipelineWidth, pipelineHeight };
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = colorAttachmentImageInfos.size();
	renderingInfo.pColorAttachments = colorAttachmentImageInfos.data();
	renderingInfo.pDepthAttachment = &depthAttachmentInfo;

	createDescriptorSets();
	createGraphicsPipeline();
}
