#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#define VOLK_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define VK_NO_PROTOTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <string>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <functional>
#include <chrono>
#include <cstdlib>
#include <vector>
#include <array>
#include <set>
#include <stb_image.h>
#include "encapVk.h"
#include "objLoader.h"
#include "MPipeline.h"
#include "MScene.h"
#include "MRenderCore.h"
#include "MInterface.h"
#include "MTracer.h"
#include "iniLoader.h"
using namespace std;

MRenderCore* renderCore;

INI_STRUCT globalConfig;
int FULL_SCREEN = 0;
int INNER_WIDTH = 800;
int INNER_HEIGHT = 600;
int OUTER_WIDTH = 1600;
int OUTER_HEIGHT = 1200;
float FAR_PLANE = 1000.0f;
float NEAR_PLANE = 0.1f;
float FOV = 45.0f;
int RADIANCE_CACHE_RAD = 512;
float CHUNK_SIZE = 102.4f;
int SSP = 8;
int SSP_2 = 20;

float lastX = 400, lastY = 300;
//x -> pitch, y -> yaw
float pitch = 0, yaw = 0;
float deltaTime;
float runingTime;
float cameraSpeed = 3.0f;
int UIEnable;
int displayID = -1;
int debugVal;
int currentSubPixel;

int main() {
	try
	{
		cameraDirection = glm::vec3(1.0f, 0.0f, 0.0f);
		initVulkan();
		mainLoop();
		renderCore->p_interface->writeStateFile();
		executeSingle("save_config");
		cleanup();
	}
	catch (runtime_error e)
	{
		std::cout << e.what() << std::endl;
	}
}

void initVulkan()
{
	loadConfig("res/config/cfg.ini");
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	auto monitor = FULL_SCREEN == 0 ? nullptr : glfwGetPrimaryMonitor();
	window = glfwCreateWindow(OUTER_WIDTH, OUTER_HEIGHT, "Vulkan", monitor, nullptr);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	executeScript("res/scripts/defaultScript.txt");
	volkInitialize();
	createInstance();
	volkLoadInstance(instance);
	setupDebugCallback();
	pickPhysicalDevice();
	createLogicalDevice();
	createSurface();
	createCommandPool();
	createSyncObjects();

	//renderCore = new MRenderCore("scenes/sanMiguelScene.txt");
	//renderCore = new MRenderCore("scenes/sponzaScene.txt");
	//renderCore->buildRenderCore();
	//renderCore->destroyRenderCore();

	//MInterface* mi = new MInterface("res/interface/defaultInterface_16x10.txt");
	string interfacePath =  MRenderCore::aspectSelect("res/interface/aspectSelector.txt");
	renderCore = new MRenderCore("res/scenes/sponzaScene.txt", "res/interface/defaultInterface_16x9.txt");
}

void mainLoop()
{
	char* cframe = (char*)malloc(100);
	float lastFrame = 0;
	float frameCount = 0;
	float lastSecond = 0;
	while (!glfwWindowShouldClose(window)) {
		frameCount++;
		if (glfwGetTime() - lastSecond > 0.5) {
			sprintf_s(cframe, 100, "%f", frameCount * 2);
			frameCount = 0;
			lastSecond = glfwGetTime();
		}
		deltaTime = glfwGetTime() - lastFrame;
		runingTime += deltaTime;
		lastFrame = glfwGetTime();
		string glTitle = "Vulkan   Fps:" + string(cframe) + "    Pos(" + to_string(-invCameraPos.x) + " , " + to_string(-invCameraPos.y) + " , " + to_string(-invCameraPos.z) + ")";
		glfwSetWindowTitle(window, glTitle.c_str());

		glfwPollEvents();
		processInput(window);
		consoleProcess();

		renderCore->drawFrame();
		renderCore->scene->sceneUpdate();
		renderCore->audio->audioUpdate();
	}
	vkDeviceWaitIdle(device);
}

void cleanup()
{
	delete renderCore;

	vkDestroySemaphore(device, imageAvailableSemaphores, nullptr);
	vkDestroyCommandPool(device, commandPool, nullptr);
	
	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	if (enableValidationLayers) {
		vkDestroyDebugUtilsMessengerEXT(instance, callback, nullptr);
	}
	vkDestroyInstance(instance, nullptr);
	glfwDestroyWindow(window);
	glfwTerminate();
}