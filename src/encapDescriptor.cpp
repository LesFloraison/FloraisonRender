#include "encapVk.h"
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vector>
#include <array>
#include <stdexcept>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

void createDescriptorSetLayout(VkDescriptorSetLayout* descriptorSetLayout)
{
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 512;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding cubemapLayoutBinding{};
	cubemapLayoutBinding.binding = 2;
	cubemapLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	cubemapLayoutBinding.descriptorCount = 512;
	cubemapLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	cubemapLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding tlasLayoutBinding{};
	tlasLayoutBinding.binding = 3;
	tlasLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	tlasLayoutBinding.descriptorCount = 1;
	tlasLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
	tlasLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding storageBuffer_1{};
	storageBuffer_1.binding = 4;
	storageBuffer_1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	storageBuffer_1.descriptorCount = 1;
	storageBuffer_1.stageFlags = VK_SHADER_STAGE_ALL;
	storageBuffer_1.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding storageBuffer_2{};
	storageBuffer_2.binding = 5;
	storageBuffer_2.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	storageBuffer_2.descriptorCount = 1;
	storageBuffer_2.stageFlags = VK_SHADER_STAGE_ALL;
	storageBuffer_2.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding sampler3DLayoutBinding_1{};
	sampler3DLayoutBinding_1.binding = 6;
	sampler3DLayoutBinding_1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	sampler3DLayoutBinding_1.descriptorCount = 1;
	sampler3DLayoutBinding_1.stageFlags = VK_SHADER_STAGE_ALL;
	sampler3DLayoutBinding_1.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding sampler3DLayoutBinding_2{};
	sampler3DLayoutBinding_2.binding = 7;
	sampler3DLayoutBinding_2.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	sampler3DLayoutBinding_2.descriptorCount = 1;
	sampler3DLayoutBinding_2.stageFlags = VK_SHADER_STAGE_ALL;
	sampler3DLayoutBinding_2.pImmutableSamplers = nullptr;

	vector<VkDescriptorBindingFlags> bindingFlags(8, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);
	VkDescriptorSetLayoutBindingFlagsCreateInfo descriptorSetLayoutBindingFlagsCreateInfo{};
	descriptorSetLayoutBindingFlagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	descriptorSetLayoutBindingFlagsCreateInfo.pBindingFlags = bindingFlags.data();
	descriptorSetLayoutBindingFlagsCreateInfo.bindingCount = bindingFlags.size();

	
	std::array<VkDescriptorSetLayoutBinding, 8> bindings = { uboLayoutBinding, samplerLayoutBinding, cubemapLayoutBinding, tlasLayoutBinding, storageBuffer_1, storageBuffer_2, sampler3DLayoutBinding_1, sampler3DLayoutBinding_2 };
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();
	layoutInfo.pNext = &descriptorSetLayoutBindingFlagsCreateInfo;

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void createDescriptorPool(VkDescriptorPool* descriptorPool)
{
	std::array<VkDescriptorPoolSize, 3> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 1;
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[2].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = 32;
	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}
