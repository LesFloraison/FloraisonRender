#pragma once
#define SHARPNESS 0.92f
#define RAD 10
#define SIG 10

//#define RADIANCE_CACHE_RAD 512
//#define CHUNK_SIZE 102.4f
//#define SSP 8
//#define SSP_2 20

#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#define UNIFROM_BUFFER_SIZE 1024
#define PUSH_CONSTS_SIZE 256
#define updateUniformBuffer(a,b) memcpy(a,b,sizeof(*b))
#include <Volk/volk.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include <string>
#include <vector>
#include <map>

struct GLFWwindow;
class MPipeline;

extern const bool enableValidationLayers;
extern int FULL_SCREEN;
extern int INNER_WIDTH;
extern int INNER_HEIGHT;
extern int OUTER_WIDTH;
extern int OUTER_HEIGHT;
extern float FAR_PLANE;
extern float NEAR_PLANE;
extern float FOV;
extern int RADIANCE_CACHE_RAD;
extern float CHUNK_SIZE;
extern int SSP;
extern int SSP_2;

extern float lastX, lastY;
extern float pitch, yaw;
extern float deltaTime;
extern float runingTime;
extern float cameraSpeed;
extern int UIEnable;
extern int displayID;
extern int debugVal;
extern int currentSubPixel;
extern glm::vec3 invCameraPos;
extern glm::vec3 historicalInvCameraPos;
extern glm::vec3 cameraDirection;
extern glm::mat4 view;
extern glm::mat4 lookat;
extern glm::mat4 proj;
extern glm::mat4 historicalVP;

extern VkSemaphore imageAvailableSemaphores;

extern VkCommandPool commandPool;
extern std::vector<VkCommandBuffer*> pCmdBuffers;

extern VkDebugUtilsMessengerEXT callback;
extern VkInstance instance;
extern VkPhysicalDevice physicalDevice;
extern VkDevice device;
extern VkQueue graphicsPresentQueue;
extern VkSurfaceKHR surface;
extern VkSwapchainKHR swapChain;
extern std::vector<VkFramebuffer> swapChainFramebuffers;
extern std::vector<VkImageView> swapChainImageViews;
extern std::vector<VkImage> swapChainImages;
extern VkFormat swapChainImageFormat;
extern VkExtent2D swapChainExtent;
extern GLFWwindow* window;

struct Vertex {
    glm::vec3 pos;
    glm::vec2 texCoord;
    glm::vec3 texIndex;
    glm::vec3 normal;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, texCoord);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texIndex);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, normal);

        return attributeDescriptions;
    }
};

struct deferredUniformBuffer {
    alignas(16) glm::vec3 cameraPos;
    alignas(16) glm::mat4 historicalVP;
    alignas(4) float runingTime;
    alignas(4) float randSeed;
    alignas(4) int pointLightSize;
    alignas(4) int debugVal;
    alignas(16) glm::vec4 lightInfos[50];
};

struct universalPushConst
{
    glm::vec4 v4;
    glm::mat4 m4;
    glm::vec4 v4_2;
    glm::mat4 m4_2;
};
void initVulkan();
void setupDebugCallback();
void mainLoop();
void cleanup();
void createInstance();
void pickPhysicalDevice();
void createLogicalDevice();
void createSurface();
void createSwapChain();
void createPipelineLayout(VkPipelineLayout* pipelineLayout,VkDescriptorSetLayout descriptorSetLayout);
void createGraphicsPipeline(VkPipeline* pipeline, std::string vertPath, std::string fragPath, VkDescriptorSetLayout descriptorSetLayout, int colorAttachmentCount);
void createGraphicsPipeline(VkPipeline* pipeline, std::string vertPath, std::string geomPath, std::string fragPath, VkDescriptorSetLayout descriptorSetLayout, int colorAttachmentCount);
void createCommandPool();
void createCommandBuffer(VkCommandBuffer* commandBuffer);
void drawFrame();
void createSyncObjects();
void createVertexBuffer(VkBuffer* vertexBuffer, VkDeviceMemory* vertexBufferMemory, void** vertexData, uint64_t bufferSize);
void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
void createDescriptorSetLayout(VkDescriptorSetLayout* descriptorSetLayout);
void createUniformBuffers(void** uniformBuffersMapped, VkDeviceMemory* uniformBuffersMemory, VkBuffer* uniformBuffer);
void createDescriptorPool(VkDescriptorPool* descriptorPool);
void createTexture(VkImageView* textureImageView, VkImage* textureImage, VkDeviceMemory* textureImageMemory, const char* path);
void createImage(VkImage* image, VkDeviceMemory* imageMemory, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
void createImageView(VkImageView* imageView, VkImage image, uint32_t mipLevels, VkFormat format, VkImageAspectFlags aspectFlags);
void createImage3D(VkImage* image, VkDeviceMemory* imageMemory, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
void createImageView3D(VkImageView* imageView, VkImage image, uint32_t mipLevels, VkFormat format, VkImageAspectFlags aspectFlags);
VkCommandBuffer beginSingleTimeCommands();
void endSingleTimeCommands(VkCommandBuffer commandBuffer);
void transitionImageLayout(VkImage image, uint32_t mipLevels, VkImageLayout oldLayout, VkImageLayout newLayout);
void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
void generateMipmaps(VkImage image, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
void createSkybox(std::string path);
void transitionImageLayout(VkImage image, uint32_t mipLevels, int layersCount, VkImageLayout oldLayout, VkImageLayout newLayout);
uint64_t getBufferDeviceAddress(VkBuffer buffer);
void createBufferWithAddress(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
void createInstanceBuffer(VkBuffer* instanceBuffer, VkDeviceMemory* instanceBufferMemory, void** instanceData, uint64_t bufferSize);
void copyImage2D(VkImage* dstImage, VkImage* srcImage2D, int texWidth, int texHeight);
void createStorageBuffer(VkBuffer* vertexBuffer, VkDeviceMemory* vertexBufferMemory, uint64_t bufferSize);
void updateStorageBuffer(VkBuffer* vertexBuffer, void** vertexData, uint64_t DataSize);
void beginRecord(VkCommandBuffer* pCmdBuffer);
void endRecordSubmit(VkCommandBuffer* pCmdBuffer, VkSemaphore* pWaitSemaphore, VkSemaphore* pSignalSemaphore);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);
void cul_mouseDir(glm::vec3* dir);
void consoleInput();
void consoleProcess();
void executeSingle(std::string executeString);
void executeScript(std::string scriptPath);
void loadConfig(std::string iniPath);

inline glm::vec2 toVec2(std::vector<float> data) {
    return glm::vec2(data[0], data[1]);
}

inline glm::vec3 toVec3(std::vector<float> data) {
    return glm::vec3(data[0], data[1], data[2]);
}