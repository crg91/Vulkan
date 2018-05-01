#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <iostream>
#include <stdexcept>
#include <functional>

#include <cassert>
#include <vector>
#include <cstring>
#include <array>
#include "camera.h"
#include "model.h"

const int WIDTH = 800;
const int HEIGHT = 600;
const std::string MODEL_PATH = "models/chalet.obj";
const std::string TEXTURE_PATH = "textures/chalet.jpg";

#ifdef NDEBUG
const bool g_enableValidationLayers = false;
#else
const bool g_enableValidationLayers = true;
#endif

struct QueueFamilyIndices
{
	int graphicsFamily = -1;
	int presentFamily = -1;
	bool isComplete()
	{
		return graphicsFamily >= 0 && presentFamily >= 0;
	}
};

// This structure contains support details for a swapchain based on the GPU card.
// This program uses this struct to decide the input properties for creating swapchain.
struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR m_capabilities;
	std::vector<VkSurfaceFormatKHR> m_surfaceFormats;
	std::vector<VkPresentModeKHR> m_presentModes;
};

struct UniformBufferObject
{
	glm::mat4 m_model;
	glm::mat4 m_view;
	glm::mat4 m_proj;
};

struct Vertex
{
	glm::vec3 m_position;
	glm::vec3 m_color;
	glm::vec2 m_texCoord;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, m_position);
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, m_color);
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, m_texCoord);
		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const
	{
		return m_position == other.m_position &&
			m_texCoord == other.m_texCoord &&
			m_color == other.m_color;
	}
};


namespace std
{
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.m_position) ^
				(hash<glm::vec3>()(vertex.m_color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.m_texCoord) << 1);
		}
	};
}

class HelloTriangleApplication
{
public:
	void run();
	HelloTriangleApplication()
		:m_vkPhysicalDevice(VK_NULL_HANDLE),
		m_vkInstance(VK_NULL_HANDLE),
		m_glfwWindow(nullptr),
		m_vkDevice(VK_NULL_HANDLE),
		m_vkGraphicsQueue(VK_NULL_HANDLE),
		m_vkPresentQueue(VK_NULL_HANDLE),
		m_vkCallback(VK_NULL_HANDLE),
		m_vkSurface(VK_NULL_HANDLE),
		m_vkSwapchain(VK_NULL_HANDLE),
		m_vkRenderPass(VK_NULL_HANDLE),
		m_vkPipelineLayout(VK_NULL_HANDLE),
		m_vkGraphicsPipeline(VK_NULL_HANDLE),
		m_vkCommandPool(VK_NULL_HANDLE),
		m_vkImageAvailableSemaphore(VK_NULL_HANDLE),
		m_vkRenderFinishedSemaphore(VK_NULL_HANDLE),
		m_vkVertexBuffer(VK_NULL_HANDLE),
		m_vkVertexBufferMemory(VK_NULL_HANDLE)
	{}

private:
	void initVulkan();
	void mainLoop();
	void createInstance();
	void createSurface();
	bool checkValidationLayersSupport();
	std::vector<const char*> getRequiredExtensions();
	void cleanup();
	void cleanupSwapchain();
	void initWindow();
	static void onWindowResize(GLFWwindow *window, int width, int height);
	static void onKeyPress(GLFWwindow *window, int key, int scanCode, int action, int mods);
	static void onMouseClick(GLFWwindow *window, int button, int action, int mods);
	static void onCursorMove(GLFWwindow *window, double xpos, double ypos);
	void setupDebugCallback();
	void pickPhysicalDevice();
	void createLogicalDevice();
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	bool ifDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionsSupport(VkPhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupportDetails(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
	VkPresentModeKHR chooseSwapChainPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities);
	void createSwapChain();
	void recreateSwapchain();
	void createSwapchainImageViews();
	void createGraphicsPipeline();
	VkShaderModule createShaderModule(const std::vector<char> &code);
	void createRenderPass();
	void createFrameBuffers();
	void createCommandPool();
	void createAndFillCommandBuffers();
	void drawFrame();
	void createSemaphores();
	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffer();
	uint32_t findMemoryType(int32_t typeFilter, VkMemoryPropertyFlags properties);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void createDescriptorSetLayout();
	void updateUniformBuffer();
	void createDescriptorPool();
	void createDescriptorSet();
	void createTextureImage();
	void createImage(uint32_t width,
		uint32_t height,
		uint32_t mipLevels,
		VkFormat imageFormat,
		VkImageTiling tiling,
		VkImageUsageFlags usageFlags,
		VkMemoryPropertyFlags properties,
		VkImage &vkImage,
		VkDeviceMemory &imageMemory
	);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	void transitionImageLayout(
		VkImage image,
		VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		uint32_t mipLevels);

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char *layerPrefix,
		const char *msg,
		void *userData);
	void createTextureImageView();
	VkImageView createImageView(
		VkImage image,
		VkFormat format,
		VkImageAspectFlags aspectFlags,
		uint32_t mipLevels);

	void createTextureSampler();
	void createDepthResources();
	VkFormat findSupportedFormat(
		const std::vector<VkFormat>&candidates,
		VkImageTiling tiling,
		VkFormatFeatureFlags features
	);
	VkFormat findDepthFormat();
	bool hasStencilComponent(VkFormat format);
	void loadModel();
	void generateMipmaps(
		VkImage image,
		int32_t texWidth,
		int32_t texHeight,
		uint32_t mipLevels
	);

	GLFWwindow *m_glfwWindow;
	VkInstance m_vkInstance;
	VkDevice m_vkDevice;
	VkPhysicalDevice m_vkPhysicalDevice;
	VkQueue m_vkGraphicsQueue;
	VkQueue m_vkPresentQueue;
	const std::vector<const char*> validationLayers = { "VK_LAYER_LUNARG_standard_validation" };
	const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	VkDebugReportCallbackEXT m_vkCallback;
	VkSurfaceKHR m_vkSurface;
	VkSwapchainKHR m_vkSwapchain;
	std::vector<VkImage> m_vkSwapchainImages;
	VkFormat m_vkSwapchainImageFormat;
	VkExtent2D m_vkSwapchainExtent;
	std::vector<VkImageView> m_vkSwapchainImageViews;
	VkRenderPass m_vkRenderPass;
	VkPipelineLayout m_vkPipelineLayout;
	VkPipeline m_vkGraphicsPipeline;
	std::vector<VkFramebuffer> m_vkSwapchainFrameBuffers;
	VkCommandPool m_vkCommandPool;
	std::vector<VkCommandBuffer> m_vkCommandBuffers;
	VkSemaphore m_vkImageAvailableSemaphore; // Image available for rendering.
	VkSemaphore m_vkRenderFinishedSemaphore; // Image available for presentation.
	VkBuffer m_vkVertexBuffer;
	VkBuffer m_vkIndexBuffer;
	VkBuffer m_vkUniformBuffer;
	VkDeviceMemory m_vkVertexBufferMemory;
	VkDeviceMemory m_vkIndexBufferMemory;
	VkDeviceMemory m_vkUniformBufferMemory;
	VkDescriptorSetLayout m_vkDescriptorSetLayout;
	VkDescriptorPool m_vkDescriptorPool;
	VkDescriptorSet m_vkDescriptorSet;
	VkImage m_vkTextureImage;
	VkDeviceMemory m_vkTextureMemory;
	VkImageView m_vkTextureImageView;
	VkSampler m_vkTextureSampler;
	VkImage m_vkDepthImage;
	VkDeviceMemory m_vkDepthImageMemory;
	VkImageView m_vkDepthImageView;
	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;
	uint32_t m_mipLevels;
	Camera m_camera;
	Model m_model;
};

