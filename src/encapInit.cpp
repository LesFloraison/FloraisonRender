#include "encapVk.h"
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vector>
#include <stdexcept>
#include <iostream>

using namespace std;

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VkDebugUtilsMessengerEXT callback;
VkInstance instance;
VkPhysicalDevice physicalDevice;
VkDevice device;
VkQueue graphicsPresentQueue;
VkSurfaceKHR surface;
VkSwapchainKHR swapChain;
std::vector<VkFramebuffer> swapChainFramebuffers;
std::vector<VkImageView> swapChainImageViews;
std::vector<VkImage> swapChainImages;
VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent;
GLFWwindow* window;
VkSemaphore imageAvailableSemaphores;

std::vector<const char*> getRequiredExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	return extensions;
}

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
	VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
	VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
	VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
	VK_KHR_RAY_QUERY_EXTENSION_NAME,
	VK_NV_MESH_SHADER_EXTENSION_NAME,
};

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

void createInstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	//VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}

}

void pickPhysicalDevice()
{
	physicalDevice = VK_NULL_HANDLE;
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	physicalDevice = devices[0];

	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

void createLogicalDevice()
{
	float queuePriority = 1.0f;
	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = 0;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeaturesEXT{};
	meshShaderFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
	meshShaderFeaturesEXT.meshShader = VK_TRUE;
	meshShaderFeaturesEXT.taskShader = VK_TRUE;
	meshShaderFeaturesEXT.pNext = nullptr;

	VkPhysicalDeviceRobustness2FeaturesEXT Robustness2Features{};
	Robustness2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
	Robustness2Features.nullDescriptor = VK_TRUE;
	Robustness2Features.pNext = &meshShaderFeaturesEXT;

	VkPhysicalDeviceRayQueryFeaturesKHR physicalDeviceRayQueryFeatures{};
	physicalDeviceRayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
	physicalDeviceRayQueryFeatures.rayQuery = VK_TRUE;
	physicalDeviceRayQueryFeatures.pNext = &Robustness2Features;

	VkPhysicalDeviceAccelerationStructureFeaturesKHR PhysicalDeviceAccelerationStructureFeaturesKHR = {};
	PhysicalDeviceAccelerationStructureFeaturesKHR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	PhysicalDeviceAccelerationStructureFeaturesKHR.accelerationStructure = VK_TRUE;
	PhysicalDeviceAccelerationStructureFeaturesKHR.pNext = &physicalDeviceRayQueryFeatures;
	
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR physicalDeviceRayTracingPipelineFeaturesKHR{};
	physicalDeviceRayTracingPipelineFeaturesKHR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
	physicalDeviceRayTracingPipelineFeaturesKHR.rayTracingPipeline = VK_TRUE;
	physicalDeviceRayTracingPipelineFeaturesKHR.pNext = &PhysicalDeviceAccelerationStructureFeaturesKHR;

	VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeature{};
	dynamicRenderingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
	dynamicRenderingFeature.dynamicRendering = VK_TRUE;
	dynamicRenderingFeature.pNext = &physicalDeviceRayTracingPipelineFeaturesKHR;

	VkPhysicalDeviceVulkan12Features feature12{};
	feature12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	feature12.descriptorBindingPartiallyBound = VK_TRUE;
	feature12.descriptorBindingVariableDescriptorCount = VK_TRUE;
	feature12.bufferDeviceAddress = VK_TRUE;
	feature12.pNext = &dynamicRenderingFeature;

	VkPhysicalDeviceFeatures2 deviceFeatures2{};
	deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures2.features.samplerAnisotropy = VK_TRUE;
	deviceFeatures2.features.geometryShader = VK_TRUE;
	deviceFeatures2.features.shaderFloat64 = VK_TRUE;
	deviceFeatures2.features.fragmentStoresAndAtomics = VK_TRUE;
	deviceFeatures2.pNext = &feature12;
	

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.queueCreateInfoCount = 1;
	createInfo.pEnabledFeatures = NULL;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	createInfo.pNext = &deviceFeatures2;

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(device, 0, 0, &graphicsPresentQueue);
}

void createSurface()
{
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

void createSwapChain()
{
	VkSurfaceCapabilitiesKHR surfaceCapability;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapability);
	VkSurfaceFormatKHR surfaceFormat;
	surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
	surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

	//VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	VkExtent2D extent = { OUTER_WIDTH, OUTER_HEIGHT };
	uint32_t imageCount = surfaceCapability.minImageCount + 1;
	if (surfaceCapability.maxImageCount > 0 && imageCount > surfaceCapability.maxImageCount) {
		imageCount = surfaceCapability.maxImageCount;
	}
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 0; // Optional
	createInfo.pQueueFamilyIndices = nullptr; // Optional
	createInfo.preTransform = surfaceCapability.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

	swapChainImageViews.resize(swapChainImages.size());
	for (uint32_t i = 0; i < swapChainImages.size(); i++) {
		createImageView(&swapChainImageViews[i], swapChainImages[i], 1, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	std::cerr << pCallbackData->pMessage << std::endl << std::endl;
	return VK_FALSE;
}

void setupDebugCallback()
{
	if (!enableValidationLayers) return;
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr; // Optional
	if (vkCreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug callback!");
	}
}

void createSyncObjects()
{
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores) != VK_SUCCESS) {
		throw std::runtime_error("failed to create synchronization objects for a frame!");
	}
}