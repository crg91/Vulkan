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
#include "utilities.h"

const int WIDTH = 800;
const int HEIGHT = 600;

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
		m_vkRenderFinishedSemaphore(VK_NULL_HANDLE)
	{
		m_models.resize(1);
		m_models[0].setModelPath("models/cube.obj");
		m_models[0].setCenter(glm::vec3(-0.5f, -0.5f, -0.5f));
		m_vkDescriptorSets.resize(1);
	}

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
	void loadModels();
	void createVertexBuffers();
	void createIndexBuffers();
	void drawFrame();
	void createSemaphores();
	void createUniformBuffer();
	uint32_t findMemoryType(int32_t typeFilter, VkMemoryPropertyFlags properties);
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
	VkDescriptorSetLayout m_vkDescriptorSetLayout;
	VkDescriptorPool m_vkDescriptorPool;
	std::vector<VkDescriptorSet> m_vkDescriptorSets;
	VkImage m_vkTextureImage;
	VkDeviceMemory m_vkTextureMemory;
	VkImageView m_vkTextureImageView;
	VkSampler m_vkTextureSampler;
	VkImage m_vkDepthImage;
	VkDeviceMemory m_vkDepthImageMemory;
	VkImageView m_vkDepthImageView;
	uint32_t m_mipLevels;
	Camera m_camera;
	std::vector<Model> m_models;
};

