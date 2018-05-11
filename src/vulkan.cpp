#include "vulkan.h"
#include <set>
#include <algorithm>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include<stb_image.h>

/**************************************************************
* Description
*		Reads the file in binary format and returns the data
*		in vector
* Returns
*		Returns bytearray
* Notes
*
**************************************************************/
static std::vector<char> readFile(const std::string &fileName)
{
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);
	
	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file");
	}

	size_t fileSize = static_cast<size_t> (file.tellg());
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	return buffer;
}

/**************************************************************
* Description
*		Proxy function which looks up the address of vkCreateDebugReportCallbackEXT 
*		function to create callback function object.
* Returns
*		VkResult.
* Notes
*
**************************************************************/
VkResult CreateDebugReportCallbackEXT(VkInstance instance,
	const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
	const VkAllocationCallbacks *pAllocator,
	VkDebugReportCallbackEXT *pCallback)
{
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

/**************************************************************
* Description
*	Proxy function which looks up the address of vkDestroyDebugReportCallbackEXT
*	function to destroy callback function object.
* Returns
*		VkResult.
* Notes
*
**************************************************************/
void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks *pAllocator)
{
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr)
	{
		func(instance, callback, pAllocator);
	}
}

/**************************************************************
* Description
*		Initializes Vulkan
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::initVulkan()
{
	createInstance();
	setupDebugCallback();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createSwapchainImageViews();
	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createCommandPool();
	createDepthResources();
	createFrameBuffers();
	createTextureImage();
	createTextureImageView();
	createTextureSampler();
	loadModels();
	createVertexBuffers();
	createIndexBuffers();
	createUniformBuffer();
	createDescriptorPool();
	createDescriptorSet();
	createAndFillCommandBuffers();
	createSemaphores();
}

void HelloTriangleApplication::mainLoop()
{
	while (!glfwWindowShouldClose(m_glfwWindow))
	{
		glfwPollEvents();
		updateUniformBuffer();
		drawFrame();
	}

	vkDeviceWaitIdle(m_vkDevice);
}

void HelloTriangleApplication::run()
{
	initWindow();
	initVulkan();
	mainLoop();
	cleanup();
}

/**************************************************************
* Description
*		Draws the frame. The drawing includes acquiring an image from
*		swapchain to submit commandbuffer to. However, since the call
*		is asynchronous, we have to specify a semaphore to be signalled
*		when the image is really available. Then we render to to the image
*		using command buffer. Once that finishes, we present it to the screen.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::drawFrame()
{
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(
								m_vkDevice,
								m_vkSwapchain,
								std::numeric_limits<uint64_t>::max(),
								m_vkImageAvailableSemaphore,
								VK_NULL_HANDLE,
								&imageIndex);

	if (VK_ERROR_OUT_OF_DATE_KHR == result)
	{
		recreateSwapchain();
		return;
	}
	else if (VK_SUCCESS != result && VK_SUBOPTIMAL_KHR != result)
	{
		throw std::runtime_error("Failed to acquire swapchain image.");
	}

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_vkImageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_vkCommandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { m_vkRenderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (VK_SUCCESS != vkQueueSubmit(m_vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE))
	{
		throw std::runtime_error("Failed to submit draw command buffer.");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	VkSwapchainKHR swaphains[] = { m_vkSwapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swaphains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	result = vkQueuePresentKHR(m_vkPresentQueue, &presentInfo);
	if (VK_ERROR_OUT_OF_DATE_KHR == result || VK_SUBOPTIMAL_KHR == result)
	{
		recreateSwapchain();
		return;
	}
	else if (VK_SUCCESS != result)
	{
		throw std::runtime_error("Failed to present swapchain image.");
	}
	vkQueueWaitIdle(m_vkPresentQueue);
}

/**************************************************************
* Description
*		Creates semaphores to be used for synchronizing operations
*		on swapchain images. There are two semaphores we will use,
*		1. Semaphore which indicates an image has been acquired for rendering
*		2. Semaphore which indicates an image has been rendered to
*			and is available for presentation.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::createSemaphores()
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	if (VK_SUCCESS != vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_vkImageAvailableSemaphore) ||
		VK_SUCCESS != vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_vkRenderFinishedSemaphore))
	{
		throw std::runtime_error("Semaphores could not be created.");
	}
}

/**************************************************************
* Description
*		Creates uniform buffer and memory. We use this buffer to
*		copy data from CPU so that it can be passed to shaders.
*		The data is fed potentially every frame and that's
*		why we are not creating a device memory which would
*		mean extra transfer per frame.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::createUniformBuffer()
{
	for (auto &model : m_models)
	{
		model.createUniformBuffer(m_vkDevice, m_vkPhysicalDevice);
	}
}

/**************************************************************
* Description
*		Creates descriptor set layout so that we can pass
*		uniform objects to shaders. Used for supplying
*		transformation matrices to vertex shader and texture
*		image to fragment shader.
*		DescriptorSetLayout specifies the type of resources to
*		bind to graphics pipeline.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboBinding = {};
	uboBinding.binding = 0;
	uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboBinding.descriptorCount = 1;
	uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboBinding, samplerLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();
	
	if (VK_SUCCESS != vkCreateDescriptorSetLayout(m_vkDevice, &layoutInfo, nullptr, &m_vkDescriptorSetLayout))
	{
		throw std::runtime_error("Could not create descriptor set layout.");
	}
}

/**************************************************************
* Description
*		Loads the models.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::loadModels()
{
	for (auto &model : m_models)
	{
		model.loadModel();
	}
}

/**************************************************************
* Description
*		Create vertex buffers.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::createVertexBuffers()
{
	for (auto &model : m_models)
	{
		model.createVertexBuffer(
			m_vkDevice,
			m_vkPhysicalDevice,
			m_vkCommandPool,
			m_vkGraphicsQueue
		);
	}
}


/**************************************************************
* Description
*		Create index buffers.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::createIndexBuffers()
{
	for (auto &model : m_models)
	{
		model.createIndexBuffer(
			m_vkDevice,
			m_vkPhysicalDevice,
			m_vkCommandPool,
			m_vkGraphicsQueue
		);
	}
}


/**************************************************************
* Description
*		Updates the uniform buffer to be fed to shader.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::updateUniformBuffer()
{
	for (int i = 0; i < m_models.size(); ++i)
	{
		UniformBufferObject ubo = {};

		// TODO : create model matrix for each model.
		ubo.m_model = m_models[i].getModelMatrix();

		ubo.m_view = m_camera.getViewMatrix();
		if (m_camera.fMouseButtonPressed())
		{
			double xPos, yPos;
			glfwGetCursorPos(m_glfwWindow, &xPos, &yPos);
			m_camera.setCurrentMousePosition(xPos, yPos);
		}
		// ubo.m_view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.m_proj = glm::perspective(glm::radians(45.0f), m_vkSwapchainExtent.width / (float)m_vkSwapchainExtent.height, 0.1f, 10.0f);
		ubo.m_proj[1][1] *= -1;
		void *data = nullptr;
		vkMapMemory(m_vkDevice, m_models[i].getUniformBufferMemory(), 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(m_vkDevice, m_models[i].getUniformBufferMemory());
	}
}

/**************************************************************
* Description
*		Create descriptor pool for descriptor sets. We have
*		two types of descriptors, unform buffer and image sampler.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 2> poolSizes = {};
	poolSizes[0].descriptorCount = 2;
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[1].descriptorCount = 1;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	
	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = 2;
	if (VK_SUCCESS != vkCreateDescriptorPool(m_vkDevice, &poolInfo, nullptr, &m_vkDescriptorPool))
	{
		throw std::runtime_error("Could not create descriptor pool");
	}
}

/**************************************************************
* Description
*		Creates descriptor set and updates the descritors to point
*		to uniform buffer and image sampler.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::createDescriptorSet()
{
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_vkDescriptorPool;
	allocInfo.descriptorSetCount = m_models.size();
	VkDescriptorSetLayout layout[2] = { m_vkDescriptorSetLayout , m_vkDescriptorSetLayout };
	allocInfo.pSetLayouts = layout;
	VkResult vkResult = vkAllocateDescriptorSets(m_vkDevice, &allocInfo, m_vkDescriptorSets.data());
	if (VK_SUCCESS != vkResult)
	{
		throw std::runtime_error("Could not create descriptor set");
	}

	for (int i = 0; i < m_vkDescriptorSets.size(); ++i)
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = m_models[i].getUniformBuffer();
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_vkTextureImageView;
		imageInfo.sampler = m_vkTextureSampler;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_vkDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].pBufferInfo = &bufferInfo;
		descriptorWrites[0].pImageInfo = nullptr;
		descriptorWrites[0].pTexelBufferView = nullptr;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = m_vkDescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(
			m_vkDevice,
			static_cast<uint32_t>(descriptorWrites.size()),
			descriptorWrites.data(),
			0,
			nullptr);
	}
}

/**************************************************************
* Description
*		Creates a texture image.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::createTextureImage()
{
	int texWidth, texHeight, texChannels;
	stbi_uc *pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
	VkDeviceSize imageSize = texWidth * texHeight * 4;
	if (!pixels)
	{
		throw std::runtime_error("Could not load texture image.");
	}
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(m_vkDevice,
		m_vkPhysicalDevice,
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory);

	void *data = nullptr;
	vkMapMemory(m_vkDevice, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(m_vkDevice, stagingBufferMemory);
	stbi_image_free(pixels);
	createImage(static_cast<uint32_t>(texWidth),
		static_cast<uint32_t>(texHeight),
		m_mipLevels,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_vkTextureImage,
		m_vkTextureMemory);

	transitionImageLayout(
		m_vkTextureImage,
		VK_FORMAT_R8G8B8A8_SNORM,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		m_mipLevels);

	copyBufferToImage(
		stagingBuffer,
		m_vkTextureImage,
		static_cast<uint32_t>(texWidth),
		static_cast<uint32_t>(texHeight));

	generateMipmaps(m_vkTextureImage, texWidth, texHeight, m_mipLevels);

	vkDestroyBuffer(m_vkDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_vkDevice, stagingBufferMemory, nullptr);
}

/**************************************************************
* Description
*		Creates an image, allocates memory for it and binds it.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::createImage(
	uint32_t width,
	uint32_t height,
	uint32_t mipLevels,
	VkFormat imageFormat,
	VkImageTiling tiling,
	VkImageUsageFlags usageFlags,
	VkMemoryPropertyFlags properties,
	VkImage & vkImage,
	VkDeviceMemory & imageMemory)
{
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = mipLevels;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = imageFormat;
	imageCreateInfo.tiling = tiling;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = usageFlags;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.flags = 0;
	if (VK_SUCCESS != vkCreateImage(m_vkDevice, &imageCreateInfo, nullptr, &vkImage))
	{
		throw std::runtime_error("Could not create texture image.");
	}

	VkMemoryRequirements memRequirements = {};
	vkGetImageMemoryRequirements(m_vkDevice, vkImage, &memRequirements);
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (VK_SUCCESS != vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &imageMemory))
	{
		throw std::runtime_error("Could not allocate memory for texture image.");
	}
	vkBindImageMemory(m_vkDevice, vkImage, imageMemory, 0);
}

/**************************************************************
* Description
*		Transition image layouts.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::transitionImageLayout(
	VkImage image,
	VkFormat format,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	uint32_t mipLevels)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(m_vkDevice, m_vkCommandPool);
	VkImageMemoryBarrier memoryBarrier = {};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	memoryBarrier.oldLayout = oldLayout;
	memoryBarrier.newLayout = newLayout;
	memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memoryBarrier.image = image;
	memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	memoryBarrier.subresourceRange.baseMipLevel = 0;
	memoryBarrier.subresourceRange.baseArrayLayer = 0;
	memoryBarrier.subresourceRange.layerCount = 1;
	memoryBarrier.subresourceRange.levelCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags dstStage;

	if (VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL == newLayout)
	{
		memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (hasStencilComponent(format))
		{
			memoryBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	if (VK_IMAGE_LAYOUT_UNDEFINED == oldLayout && VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL == newLayout)
	{
		memoryBarrier.srcAccessMask = 0;
		memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL == oldLayout && VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL == newLayout)
	{
		memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (VK_IMAGE_LAYOUT_UNDEFINED == oldLayout && VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL == newLayout)
	{
		memoryBarrier.srcAccessMask = 0;
		memoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
	{
		throw std::runtime_error("Unsupported transition layout.");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage,
		dstStage,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&memoryBarrier);

	endSingleTimeCommands(m_vkDevice, m_vkCommandPool, m_vkGraphicsQueue, commandBuffer);
}

/**************************************************************
* Description
*		Helper functoin to copy buffer data to an image.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(m_vkDevice, m_vkCommandPool);

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferImageHeight = 0;
	region.bufferRowLength = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };
	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	endSingleTimeCommands(m_vkDevice, m_vkCommandPool, m_vkGraphicsQueue, commandBuffer);
}

/**************************************************************
* Description
*		Finds memory types
* Returns
*		void
* Notes
*
**************************************************************/
uint32_t HelloTriangleApplication::findMemoryType(int32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &memoryProperties);
	for (int32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
	{
		if ((typeFilter & (1 << i)) &&
			(memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	return uint32_t();
}

/**************************************************************
* Description
*		Creates Vulkan Instance. Enables validation layers if
*		needed. We check for the extensions needed here.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::createInstance()
{
	if (g_enableValidationLayers && !checkValidationLayersSupport())
	{
		throw std::runtime_error("Validation layers requested, but not available!");
	}

	VkApplicationInfo vkAppInfo = {};
	vkAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	vkAppInfo.pApplicationName = "Hello Triangle";
	vkAppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	vkAppInfo.pEngineName = "No Engine";
	vkAppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	vkAppInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo vkCreateInfo = {};
	vkCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	vkCreateInfo.pApplicationInfo = &vkAppInfo;

	auto vkExtensions = getRequiredExtensions();
	vkCreateInfo.enabledExtensionCount = static_cast<uint32_t>(vkExtensions.size());
	vkCreateInfo.ppEnabledExtensionNames = vkExtensions.data();
	if (g_enableValidationLayers)
	{
		vkCreateInfo.enabledLayerCount = validationLayers.size();
		vkCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		vkCreateInfo.enabledLayerCount = 0;
	}

	VkResult vkResult = vkCreateInstance(&vkCreateInfo, nullptr /* callback */, &m_vkInstance);

	if (VK_SUCCESS != vkResult)
	{
		throw std::runtime_error("Failed to create vulkan instance!!");
	}

	assert(m_vkInstance);

	// Print all the vulkan extensions supported.
	//
	uint32_t extensionCount = 0;
	vkResult = vkEnumerateInstanceExtensionProperties(nullptr /*pLayerName*/, &extensionCount, nullptr /*properties*/);
	assert(VK_SUCCESS == vkResult);

	std::vector<VkExtensionProperties> extensions(extensionCount);

	vkEnumerateInstanceExtensionProperties(nullptr /*pLayerName*/, &extensionCount, extensions.data());
	assert(VK_SUCCESS == vkResult);

#ifndef NDEBUG
	for (const VkExtensionProperties extension : extensions)
	{
		std::cout << "\t" << extension.extensionName << std::endl;
	}
#endif
}

/**************************************************************
* Description
*		Checks if validation layers are supported.
* Returns
*		true/false
* Notes
*
**************************************************************/
bool HelloTriangleApplication::checkValidationLayersSupport()
{
	uint32_t layerCount = 0;
	VkResult vkResult = VK_SUCCESS;
	vkResult = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	assert(VK_SUCCESS == vkResult);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkResult = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
	assert(VK_SUCCESS == vkResult);

	for (const char *layerName : validationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (0 == strcmp(layerName, layerProperties.layerName))
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}

/**************************************************************
* Description
*		Gets the list of required extensions for vulkan.
*		The extensions required are extensions needed for
*		GLFW to support vulkan surfaces and for validation layers.
* Returns
*		The list of extensions.
* Notes
*
**************************************************************/
std::vector<const char*> HelloTriangleApplication::getRequiredExtensions()
{
	std::vector<const char*> extensions;
	const char** glfwExtensions = nullptr;
	unsigned int glfwExtensionCount = 0;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	assert(glfwExtensionCount);

	for (unsigned int iExtension = 0; iExtension < glfwExtensionCount; ++iExtension)
	{
		extensions.push_back(glfwExtensions[iExtension]);
	}

	if (g_enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return extensions;
}

/**************************************************************
* Description
*		Cleans up glfw and vulkan objects.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::cleanup()
{
	cleanupSwapchain();
	vkDestroyDescriptorPool(m_vkDevice, m_vkDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_vkDevice, m_vkDescriptorSetLayout, nullptr);
	vkDestroySampler(m_vkDevice, m_vkTextureSampler, nullptr);
	vkDestroyImageView(m_vkDevice, m_vkTextureImageView, nullptr);
	vkDestroyImage(m_vkDevice, m_vkTextureImage, nullptr);
	vkFreeMemory(m_vkDevice, m_vkTextureMemory, nullptr);

	for (auto model : m_models)
	{
		model.cleanup(m_vkDevice);
	}
	vkDestroySemaphore(m_vkDevice, m_vkImageAvailableSemaphore, nullptr);
	vkDestroySemaphore(m_vkDevice, m_vkRenderFinishedSemaphore, nullptr);
	vkDestroyCommandPool(m_vkDevice, m_vkCommandPool, nullptr);

	vkDestroyDevice(m_vkDevice, nullptr);
	vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);
	DestroyDebugReportCallbackEXT(m_vkInstance, m_vkCallback, nullptr /*pAllocator*/);
	vkDestroyInstance(m_vkInstance, nullptr);
	glfwDestroyWindow(m_glfwWindow);
	glfwTerminate();
}

/**************************************************************
* Description
*		Cleans up swap chain resources.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::cleanupSwapchain()
{
	vkDestroyImageView(m_vkDevice, m_vkDepthImageView, nullptr);
	vkDestroyImage(m_vkDevice, m_vkDepthImage, nullptr);
	vkFreeMemory(m_vkDevice, m_vkDepthImageMemory, nullptr);

	for (auto framebuffer : m_vkSwapchainFrameBuffers)
	{
		vkDestroyFramebuffer(m_vkDevice, framebuffer, nullptr);
	}

	vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, m_vkCommandBuffers.size(), m_vkCommandBuffers.data());
	vkDestroyPipeline(m_vkDevice, m_vkGraphicsPipeline, nullptr);

	vkDestroyPipelineLayout(m_vkDevice, m_vkPipelineLayout, nullptr);
	vkDestroyRenderPass(m_vkDevice, m_vkRenderPass, nullptr);

	for (auto imageView : m_vkSwapchainImageViews)
	{
		vkDestroyImageView(m_vkDevice, imageView, nullptr);
	}

	vkDestroySwapchainKHR(m_vkDevice, m_vkSwapchain, nullptr /*pAllocator*/);
}

/**************************************************************
* Description
*		Initializes the GLFW window for Vulkan.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::initWindow()
{
	int glfwReturnCode = glfwInit();
	if (GLFW_FALSE == glfwReturnCode)
	{
		throw std::runtime_error("GLFW library could not be initialized.");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	// TODO : Try projecting to different monitor.
	//
	m_glfwWindow = glfwCreateWindow(WIDTH,
		HEIGHT,
		"glfw window" /* title */,
		nullptr /* monitor */,
		nullptr /* share */);

	assert(m_glfwWindow);

	// We set pointer to the class so that it can retrieved
	// in callback function when window is resized.
	//
	glfwSetWindowUserPointer(m_glfwWindow, this);

	glfwSetWindowSizeCallback(m_glfwWindow, HelloTriangleApplication::onWindowResize);
	glfwSetKeyCallback(m_glfwWindow, HelloTriangleApplication::onKeyPress);
	glfwSetMouseButtonCallback(m_glfwWindow, HelloTriangleApplication::onMouseClick);
}

/**************************************************************
* Description
*		The static callback function to use when window is resized.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::onWindowResize(GLFWwindow * window, int width, int height)
{
	HelloTriangleApplication *thisApp = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
	thisApp->recreateSwapchain();
}

/**************************************************************
* Description
*		The callback function for keyboard events.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::onKeyPress(
	GLFWwindow *window,
	int key,
	int scanCode,
	int action,
	int mods)
{
	HelloTriangleApplication *app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
	switch (key)
	{
	case GLFW_KEY_ESCAPE:
		glfwSetWindowShouldClose(window, true);
		break;
	case GLFW_KEY_UP:
	case GLFW_KEY_DOWN:
		if (GLFW_PRESS == action || GLFW_REPEAT == action)
		{
			app->m_models[0].setXKeyPressed(true);
			if (GLFW_KEY_UP == key)
			{
				app->m_models[0].fXDirectionPositive(false);
			}
			else
			{
				app->m_models[0].fXDirectionPositive(true);
			}
		}
		else
		{
			app->m_models[0].setXKeyPressed(false);
		}

		break;
	case GLFW_KEY_RIGHT:
	case GLFW_KEY_LEFT:
		if (GLFW_PRESS == action || GLFW_REPEAT == action)
		{
			app->m_models[0].setYKeyPressed(true);
			if (GLFW_KEY_LEFT == key)
			{
				app->m_models[0].fYDirectionPositive(false);
			}
			else
			{
				app->m_models[0].fYDirectionPositive(true);
			}
		}
		else
		{
			app->m_models[0].setYKeyPressed(false);
		}
		break;
	case GLFW_KEY_M:
	case GLFW_KEY_N:
		if (GLFW_PRESS == action || GLFW_REPEAT == action)
		{
			app->m_models[0].setZKeyPressed(true);
			if (GLFW_KEY_N == key)
			{
				app->m_models[0].fZDirectionPositive(true);
			}
			else
			{
				app->m_models[0].fZDirectionPositive(false);
			}
		}
		else
		{
			app->m_models[0].setZKeyPressed(false);
		}
		break;
	}
}

/**************************************************************
* Description
*		The glfw callback function for mouse button clicks.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::onMouseClick(GLFWwindow * window, int button, int action, int mods)
{
	HelloTriangleApplication *app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
	if (GLFW_MOUSE_BUTTON_LEFT == button)
	{
		if (GLFW_PRESS == action)
		{
			app->m_camera.setMouseButtonPressed(true);
			double xPos, yPos;
			glfwGetCursorPos(window, &xPos, &yPos);
			app->m_camera.setInitialMousePosition(xPos, yPos);
		}
		else
		{
			app->m_camera.setMouseButtonPressed(false);
		}
	}
}

/**************************************************************
* Description
*		The glfw callback function for when the cursor is on the screen.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::onCursorMove(GLFWwindow * window, double xpos, double ypos)
{
	HelloTriangleApplication *app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
	if (app->m_camera.fMouseButtonPressed())
	{
		app->m_camera.setCurrentMousePosition(xpos, ypos);
	}
}

/**************************************************************
* Description
*		The callback function for validation layers.
* Returns
*		void
* Notes
*
**************************************************************/
VKAPI_ATTR VkBool32 VKAPI_CALL HelloTriangleApplication::debugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char *layerPrefix,
	const char *msg,
	void *userData)
{
	std::cerr << "validation layer: " << msg << std::endl;
	return VK_FALSE;
}

/**************************************************************
* Description
*		Creates the texture image view for the texture image.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::createTextureImageView()
{
	m_vkTextureImageView = createImageView(
							m_vkTextureImage,
							VK_FORMAT_R8G8B8A8_UNORM,
							VK_IMAGE_ASPECT_COLOR_BIT,
							m_mipLevels);
}

/**************************************************************
* Description
*		Helper function to create image view based on image, format,
*		aspect flags and miplevels.
* Returns
*		void
* Notes
*
**************************************************************/
VkImageView HelloTriangleApplication::createImageView(
	VkImage image,
	VkFormat format,
	VkImageAspectFlags aspectFlags,
	uint32_t mipLevels)
{
	VkImageView imageView;
	VkImageViewCreateInfo imageViewInfo = {};
	imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewInfo.image = image;
	imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewInfo.format = format;
	imageViewInfo.subresourceRange.aspectMask = aspectFlags;
	imageViewInfo.subresourceRange.baseMipLevel = 0;
	imageViewInfo.subresourceRange.layerCount = 1;
	imageViewInfo.subresourceRange.levelCount = mipLevels;
	imageViewInfo.subresourceRange.baseArrayLayer = 0;

	if (VK_SUCCESS != vkCreateImageView(m_vkDevice, &imageViewInfo, nullptr, &imageView))
	{
		throw std::runtime_error("Could not create image view.");
	}
	return imageView;
}

/**************************************************************
* Description
*		Creates texture sampler for texture access in shader.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::createTextureSampler()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<float>(m_mipLevels);
	if (VK_SUCCESS != vkCreateSampler(m_vkDevice, &samplerInfo, nullptr, &m_vkTextureSampler))
	{
		throw std::runtime_error("Could not create texture sampler.");
	}

}

/**************************************************************
* Description
*		Creates the relevant resource for depth buffering.
*		Includes creating image, memory for image and image view.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::createDepthResources()
{
	VkFormat depthFormat = findDepthFormat();
	createImage(
		m_vkSwapchainExtent.width,
		m_vkSwapchainExtent.height,
		1 /*mipLevels*/,
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_vkDepthImage,
		m_vkDepthImageMemory);
	m_vkDepthImageView = createImageView(m_vkDepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1 /*mipLevels*/);

	// Transition the depth image to right layout.
	//
	transitionImageLayout(
		m_vkDepthImage,
		depthFormat,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		1 /*mipLevels*/);
}

/**************************************************************
* Description
*		Selects the right format based on input properties.
* Returns
*		VkFormat
* Notes
*
**************************************************************/
VkFormat HelloTriangleApplication::findSupportedFormat(
	const std::vector<VkFormat>& candidates,
	VkImageTiling tiling,
	VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_vkPhysicalDevice, format, &props);
		if (VK_IMAGE_TILING_LINEAR == tiling && ((props.linearTilingFeatures & features) == features))
		{
			return format;
		}
		else if (VK_IMAGE_TILING_OPTIMAL == tiling && ((props.optimalTilingFeatures & features) == features))
		{
			return format;
		}
		throw std::runtime_error("Could not find the right format.");
	}
}

/**************************************************************
* Description
*		Finds the right depth format;
* Returns
*		VkFormat
* Notes
*
**************************************************************/
VkFormat HelloTriangleApplication::findDepthFormat()
{
	return findSupportedFormat(
	{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

/**************************************************************
* Description
*		Checks if the format has stencil component.
* Returns
*		boolean
* Notes
*
**************************************************************/
bool HelloTriangleApplication::hasStencilComponent(VkFormat format)
{
	return VK_FORMAT_D32_SFLOAT_S8_UINT == format || VK_FORMAT_D24_UNORM_S8_UINT == format;
}

/**************************************************************
* Description
*		Generates mipmaps
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::generateMipmaps(
	VkImage image,
	int32_t texWidth,
	int32_t texHeight,
	uint32_t mipLevels)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(m_vkDevice, m_vkCommandPool);
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;
	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;
	for (uint32_t i = 1; i < m_mipLevels; ++i)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&barrier);
		VkImageBlit blit = {};
		blit.srcOffsets[0] = { 0,0,0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0,0,0 };
		blit.dstOffsets[1] = { mipWidth / 2, mipHeight / 2, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;
		vkCmdBlitImage(
			commandBuffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&blit,
			VK_FILTER_LINEAR);
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&barrier);

		if (mipWidth > 1)
		{
			mipWidth /= 2;
		}
		if (mipHeight > 1)
		{
			mipHeight /= 2;
		}
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&barrier);

	endSingleTimeCommands(m_vkDevice, m_vkCommandPool, m_vkGraphicsQueue, commandBuffer);
}

/**************************************************************
* Description
*		Sets up the debug callbacks for validation layers.
*		Validations layers need a callback which they will
*		call after each function has been invoked.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::setupDebugCallback()
{
	if (!g_enableValidationLayers)
	{
		return;
	}

	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = debugCallback;
	if (VK_SUCCESS != CreateDebugReportCallbackEXT(m_vkInstance, &createInfo, nullptr /* pAllocator */, &m_vkCallback))
	{
		throw std::runtime_error("failed to setup debug callback!");
	}
}

/**************************************************************
* Description
*		Creates a vulkan window surface. We are using glfw library
*		to get a vulkan window surface.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::createSurface()
{
	if (VK_SUCCESS != glfwCreateWindowSurface(m_vkInstance, m_glfwWindow, nullptr /* allocation callback */, &m_vkSurface))
	{
		throw std::runtime_error("Could not create a window surface.");
	}
}

/**************************************************************
* Description
*		Pick the graphics card for vulkan. We enumerate over
*		the available cards and choose the one which satisfies
*		our requirements.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::pickPhysicalDevice()
{
	uint32_t physicalDeviceCount = 0;
	if (VK_SUCCESS != vkEnumeratePhysicalDevices(m_vkInstance, &physicalDeviceCount, nullptr))
	{
		throw std::runtime_error("failed to enumerate physical devices.");
	}

	if (!physicalDeviceCount)
	{
		throw std::runtime_error("No GPU found with Vulkan support! :(");
	}

	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(m_vkInstance, &physicalDeviceCount, physicalDevices.data());

	for (const auto& device : physicalDevices)
	{
		if (ifDeviceSuitable(device))
		{
			m_vkPhysicalDevice = device;
			break;
		}
	}

	if (VK_NULL_HANDLE == m_vkPhysicalDevice)
	{
		throw std::runtime_error("Failed to find a suitable GPU!");
	}
}

/**************************************************************
* Description
*		Check if the device is okay for us to use.
* Returns
*		true/false
* Notes
*
**************************************************************/
bool HelloTriangleApplication::ifDeviceSuitable(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	QueueFamilyIndices indices = findQueueFamilies(device);

	bool fExtensionsSupported = checkDeviceExtensionsSupport(device);
	bool fSwapChainSupportEnough = false;

	if (fExtensionsSupported)
	{
		SwapChainSupportDetails swapChainSupportDetails = querySwapChainSupportDetails(device);
		fSwapChainSupportEnough = !swapChainSupportDetails.m_surfaceFormats.empty()
								&& !swapChainSupportDetails.m_presentModes.empty();
	}
	
	return indices.isComplete() &&
		fExtensionsSupported &&
		fSwapChainSupportEnough &&
		deviceFeatures.samplerAnisotropy;
}

/**************************************************************
* Description
*		Checks if the device supports required extensions.
* Returns
*		true/false
* Notes
*
**************************************************************/
bool HelloTriangleApplication::checkDeviceExtensionsSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount = 0;
	if (VK_SUCCESS != vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr))
	{
		throw std::runtime_error("Could not enumerate device extensions");
	}
	
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	// Check if the required extensions are present in available extensions.
	// Make a copy of the required extensions for in-place updates.
	//
	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (auto &availableExtension : availableExtensions)
	{
		requiredExtensions.erase(availableExtension.extensionName);
	}

	return requiredExtensions.empty();
}

/**************************************************************
* Description
*		Finds the indices of required queue families for the device.
* Returns
*		indices of queueFamilies
* Notes
*		queues supporting graphics and presentation are required.
*
**************************************************************/
QueueFamilyIndices HelloTriangleApplication::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	if (!queueFamilyCount)
	{
		throw std::runtime_error("No queue family found.");
	}

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_vkSurface, &presentSupport);
		if (queueFamily.queueCount > 0 && presentSupport)
		{
			indices.presentFamily = i;
		}

		if (indices.isComplete())
		{
			break;
		}

		i++;
	}

	return indices;
}

/**************************************************************
* Description
		Obtains the swap chain support details for a physical device.
* Returns
*		struct containing details about swap chain support.
* Notes
*
**************************************************************/
SwapChainSupportDetails HelloTriangleApplication::querySwapChainSupportDetails(VkPhysicalDevice device)
{
	SwapChainSupportDetails supportDetails;

	if (VK_SUCCESS != vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_vkSurface, &supportDetails.m_capabilities))
	{
		throw std::runtime_error("Could not obtain swap chain surface capabilites.");
	}

	uint32_t formatCount = 0;

	if (VK_SUCCESS != vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_vkSurface, &formatCount, nullptr))
	{
		throw std::runtime_error("Could not query swap chain surface format details.");
	}

	if (formatCount)
	{
		supportDetails.m_surfaceFormats.resize(formatCount);
	}

	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_vkSurface, &formatCount, supportDetails.m_surfaceFormats.data());

	uint32_t presentModesCount = 0;

	if (VK_SUCCESS != vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_vkSurface, &presentModesCount, nullptr))
	{
		throw std::runtime_error("Could not query swap chain surface present modes.");
	}

	if (presentModesCount)
	{
		supportDetails.m_presentModes.resize(presentModesCount);
	}

	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_vkSurface, &presentModesCount, supportDetails.m_presentModes.data());

	return supportDetails;
}

/**************************************************************
* Description
*		Chooses the swap chain surface format to be used.
* Returns
*		surface format.
* Notes
*
**************************************************************/
VkSurfaceFormatKHR HelloTriangleApplication::chooseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	// If there is no constraint on the surface format, we will use the ideal surface format for us.
	//
	if (1 == availableFormats.size() && VK_FORMAT_UNDEFINED == availableFormats[0].format)
	{
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
	}
	
	for (const auto& availableFormat : availableFormats)
	{
		if (VK_FORMAT_B8G8R8A8_UNORM == availableFormat.format
			&& VK_COLORSPACE_SRGB_NONLINEAR_KHR == availableFormat.colorSpace)
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

/**************************************************************
* Description
*		Chooses the present mode for the swap chain.
* Returns
*		Presentation mode.
* Notes
*		VK_PRESENT_MODE_MAILBOX_KHR is the most desirable present mode
*		followed by VK_PRESENT_MODE_IMMEDIATE_KHR and VK_PRESENT_MODE_FIFO_KHR
*		respectively.
*
**************************************************************/
VkPresentModeKHR HelloTriangleApplication::chooseSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	VkPresentModeKHR bestPresentationMode = VK_PRESENT_MODE_FIFO_KHR;
	for (const auto& presentationMode : availablePresentModes)
	{
		if (VK_PRESENT_MODE_MAILBOX_KHR == presentationMode)
		{
			return presentationMode;
		}
		else if (VK_PRESENT_MODE_IMMEDIATE_KHR == presentationMode)
		{
			bestPresentationMode = presentationMode;
		}
	}
	return bestPresentationMode;
}

/**************************************************************
* Description
		Choose swapchain extent
* Returns
*		Extent
* Notes
*
**************************************************************/
VkExtent2D HelloTriangleApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR & surfaceCapabilities)
{
	VkExtent2D actualExtent = { WIDTH, HEIGHT };

	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		// Vulkan has specified the desired extent for swapchain and we need to honor that.
		//
		return surfaceCapabilities.currentExtent;
	}
	else
	{
		// We are free to choose the extent we desire with the condition that it should
		// fall within minimum and maximum allowed values.
		//
		actualExtent.width = std::max(surfaceCapabilities.minImageExtent.width,
			std::min(surfaceCapabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(surfaceCapabilities.minImageExtent.height,
			std::min(surfaceCapabilities.maxImageExtent.height, actualExtent.height));
		return actualExtent;
	}

	return actualExtent;
}

/**************************************************************
* Description
*		Creates the swap chain. Once created, we also retrieve
*		the images associated with the swapchain.
* Returns
*		void
* Notes
*		The assumption is that physical device is already created.
*
**************************************************************/
void HelloTriangleApplication::createSwapChain()
{
	assert(VK_NULL_HANDLE != m_vkPhysicalDevice);
	SwapChainSupportDetails swapChainSupportDetails = querySwapChainSupportDetails(m_vkPhysicalDevice);
	VkSurfaceFormatKHR surfaceFormat = chooseSwapChainSurfaceFormat(swapChainSupportDetails.m_surfaceFormats);
	VkPresentModeKHR presentMode = chooseSwapChainPresentMode(swapChainSupportDetails.m_presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupportDetails.m_capabilities);

	// Keep image count to be one more than minimum required swapchain images.
	//
	uint32_t imageCount = swapChainSupportDetails.m_capabilities.minImageCount + 1;
	if (swapChainSupportDetails.m_capabilities.maxImageCount > 0
		&& imageCount > swapChainSupportDetails.m_capabilities.maxImageCount)
	{
		// Ensure that we do not exceed maximum allowed images for swapchain.
		//
		imageCount = swapChainSupportDetails.m_capabilities.maxImageCount;
	}

	m_vkSwapchainExtent = extent;
	m_vkSwapchainImageFormat = surfaceFormat.format;

	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = m_vkSurface;
	swapChainCreateInfo.imageExtent = extent;
	swapChainCreateInfo.imageFormat = surfaceFormat.format;
	swapChainCreateInfo.minImageCount = imageCount;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = findQueueFamilies(m_vkPhysicalDevice);
	uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };
	if (indices.graphicsFamily != indices.presentFamily)
	{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	swapChainCreateInfo.preTransform = swapChainSupportDetails.m_capabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.presentMode = presentMode;
	swapChainCreateInfo.clipped = VK_TRUE;
	swapChainCreateInfo.oldSwapchain = nullptr;

	if (VK_SUCCESS != vkCreateSwapchainKHR(m_vkDevice, &swapChainCreateInfo, nullptr, &m_vkSwapchain))
	{
		throw std::runtime_error("Swapchain creation failed.");
	}

	uint32_t swapchainImageCount = 0;
	if (VK_SUCCESS != vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapchain, &imageCount, nullptr))
	{
		throw std::runtime_error("Could not query swap chain images.");
	}
	
	m_vkSwapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapchain, &imageCount, m_vkSwapchainImages.data());
}

/**************************************************************
* Description
*		Recreates the swapchain.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::recreateSwapchain()
{
	int width, height;
	glfwGetWindowSize(m_glfwWindow, &width, &height);
	if (0 == width || 0 == height)
	{
		return;
	}

	vkDeviceWaitIdle(m_vkDevice);
	cleanupSwapchain();
	createSwapChain();
	createSwapchainImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createDepthResources();
	createFrameBuffers();
	createAndFillCommandBuffers();
}

/**************************************************************
* Description
*		Create swapchain image views. There is one imageview
*		for each swapchain image.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::createSwapchainImageViews()
{
	m_vkSwapchainImageViews.resize(m_vkSwapchainImages.size());
	for (size_t i = 0; i < m_vkSwapchainImages.size(); ++i)
	{
		m_vkSwapchainImageViews[i] = createImageView(
										m_vkSwapchainImages[i],
										m_vkSwapchainImageFormat,
										VK_IMAGE_ASPECT_COLOR_BIT,
										1 /*mipLevels*/);
	}
}

/**************************************************************
* Description
*		Creates the graphics pipeline. This involves setting up
*		shaders and other nonprogramming pipeline stages. These
*		include
*		1. Vertex and Fragment Shader
*		2. Input Assembler
*		3. Viewport info
*		4. Rasterization
*		5. Multisampling
*		6. Depth/Stencil Buffer
*		7. Color/Alpha Blending
*		8. Descriptor set layout for uniform object
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::createGraphicsPipeline()
{
	auto vertShaderCode = readFile("shaders/vert.spv");
	auto fragShaderCode = readFile("shaders/frag.spv");
	auto vertShaderModule = createShaderModule(vertShaderCode);
	auto fragShaderModule = createShaderModule(fragShaderCode);
	
	VkPipelineShaderStageCreateInfo  vertShaderStageCreateInfo = {};
	vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageCreateInfo.module = vertShaderModule;
	vertShaderStageCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo  fragShaderStageCreateInfo = {};
	fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageCreateInfo.module = fragShaderModule;
	fragShaderStageCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStageCreateInfos[] = { vertShaderStageCreateInfo, fragShaderStageCreateInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_vkSwapchainExtent.width);
	viewport.height = static_cast<float>(m_vkSwapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissorRectangle = {};
	scissorRectangle.extent = m_vkSwapchainExtent;
	scissorRectangle.offset = { 0, 0 };

	VkPipelineViewportStateCreateInfo viewportStateInfo = {};
	viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.pViewports = &viewport;
	viewportStateInfo.scissorCount = 1;
	viewportStateInfo.pScissors = &scissorRectangle;

	VkPipelineRasterizationStateCreateInfo rasterizerInfo = {};
	rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerInfo.depthClampEnable = VK_FALSE;
	rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerInfo.lineWidth = 1.0f;
	rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizerInfo.depthBiasEnable = VK_FALSE;
	rasterizerInfo.depthBiasConstantFactor = 0.0f;
	rasterizerInfo.depthBiasClamp = 0.0f;
	rasterizerInfo.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisamplingInfo = {};
	multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingInfo.sampleShadingEnable = VK_FALSE;
	multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisamplingInfo.minSampleShading = 1.0f;
	multisamplingInfo.pSampleMask = nullptr;
	multisamplingInfo.alphaToCoverageEnable = VK_FALSE;
	multisamplingInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {};
	depthStencil.back = {};

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
											VK_COLOR_COMPONENT_G_BIT |
											VK_COLOR_COMPONENT_B_BIT |
											VK_COLOR_COMPONENT_A_BIT;

	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlendingInfo = {};
	colorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingInfo.logicOpEnable = VK_FALSE;
	colorBlendingInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendingInfo.attachmentCount = 1;
	colorBlendingInfo.pAttachments = &colorBlendAttachment;
	colorBlendingInfo.blendConstants[0] = 0.0f;
	colorBlendingInfo.blendConstants[1] = 0.0f;
	colorBlendingInfo.blendConstants[2] = 0.0f;
	colorBlendingInfo.blendConstants[3] = 0.0f;

	// NOTE : Can we not create descriptor set layout here?
	//
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_vkDescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = 0;

	if (VK_SUCCESS != vkCreatePipelineLayout(m_vkDevice, &pipelineLayoutInfo, nullptr, &m_vkPipelineLayout))
	{
		throw std::runtime_error("Could not create pipeline layout.");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStageCreateInfos;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineInfo.pViewportState = &viewportStateInfo;
	pipelineInfo.pRasterizationState = &rasterizerInfo;
	pipelineInfo.pMultisampleState = &multisamplingInfo;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlendingInfo;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = m_vkPipelineLayout;
	pipelineInfo.renderPass = m_vkRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;
	pipelineInfo.pDepthStencilState = &depthStencil;

	if (VK_SUCCESS != vkCreateGraphicsPipelines(m_vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_vkGraphicsPipeline))
	{
		throw std::runtime_error("Could not create graphics pipeline");
	}

	vkDestroyShaderModule(m_vkDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(m_vkDevice, fragShaderModule, nullptr);
}

/**************************************************************
* Description
*		Creates vulkan logical device and retrieves the queues.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies(m_vkPhysicalDevice);
	
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> queueFamilyIndices = { indices.graphicsFamily, indices.presentFamily };

	float queuePriority = 1.0f;
	
	for (int queueFamilyIndex : queueFamilyIndices)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}
	
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (g_enableValidationLayers)
	{
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		deviceCreateInfo.enabledLayerCount = 0;
	}

	if (VK_SUCCESS != vkCreateDevice(m_vkPhysicalDevice, &deviceCreateInfo, nullptr /* pAllocator */, &m_vkDevice))
	{
		throw std::runtime_error("Failed to create logical device!");
	}

	vkGetDeviceQueue(m_vkDevice, indices.graphicsFamily, 0, &m_vkGraphicsQueue);
	vkGetDeviceQueue(m_vkDevice, indices.presentFamily, 0, &m_vkPresentQueue);
}

/**************************************************************
* Description
*		Creates shader module for the byte code.
* Returns
*		VkShaderModule
* Notes
*
**************************************************************/
VkShaderModule HelloTriangleApplication::createShaderModule(const std::vector<char> &code)
{
	VkShaderModule vkShaderModule;
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = code.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	if (VK_SUCCESS != vkCreateShaderModule(m_vkDevice, &shaderModuleCreateInfo, nullptr, &vkShaderModule))
	{
		throw std::runtime_error("Could not create shader module.");
	}

	return vkShaderModule;
}

/**************************************************************
* Description
*		Create render pass. This involes creation of attachments,
*		color and depth in our case. We also create attachment 
*		references which are used in subpass.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::createRenderPass()
{
	// Color attachment is representation of swapchain images
	//
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = m_vkSwapchainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// We need to create an attachment reference which can be used
	// by the subpass.
	//
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// We need to create an attachment reference which can be used
	// by the subpass.
	//
	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// We must have atleast one subpass and hence we need to create this.
	// It references the attachment references we created.
	//
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	// Here we are creating a depdency for the subpass.
	// We won't transition to current subpass operatoins from the previous implicit
	// subpass unless color attachment stage is available.
	// This means that we will come to current subpass only when images are ready to
	// be rendered by the graphics pipeline.
	//
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (VK_SUCCESS != vkCreateRenderPass(m_vkDevice, &renderPassInfo, nullptr, &m_vkRenderPass))
	{
		throw std::runtime_error("Could not create render pass.");
	}
}

/**************************************************************
* Description
*		Creates frame buffers. There is one framebuffer for each
*		image view of color attachment. 
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::createFrameBuffers()
{
	m_vkSwapchainFrameBuffers.resize(m_vkSwapchainImageViews.size());
	for (size_t i = 0; i < m_vkSwapchainImageViews.size(); ++i)
	{
		std::array<VkImageView,2> attachments = { m_vkSwapchainImageViews[i], m_vkDepthImageView};
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_vkRenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = m_vkSwapchainExtent.width;
		framebufferInfo.height = m_vkSwapchainExtent.height;
		framebufferInfo.layers = 1;

		if (VK_SUCCESS != vkCreateFramebuffer(m_vkDevice, &framebufferInfo, nullptr, &m_vkSwapchainFrameBuffers[i]))
		{
			throw std::runtime_error("Could not create framebuffer");
		}
	}
}

/**************************************************************
* Description
*		Create command pool. The pool is manager for all command
*		buffers. The command pool is linked to graphics family index.
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_vkPhysicalDevice);
	VkCommandPoolCreateInfo commandPoolInfo = {};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	commandPoolInfo.flags = 0;
	if (VK_SUCCESS != vkCreateCommandPool(m_vkDevice, &commandPoolInfo, nullptr, &m_vkCommandPool))
	{
		throw std::runtime_error("Could not create command pool.");
	}
}

/**************************************************************
* Description
*		Create command buffers and record them. Each command buffer
*		is linked to a framebuffer, so we have to create one for each
*		framebuffer.
*		We also record drawing commands to the command buffers. These include
*		1. Start Command buffer
*		2. Start Render Pass
*		3. Bind the graphics pipeline
*		4. Bind Vertex Buffer, Index Buffer and Descriptor Sets
*		5. Draw command!
* Returns
*		void
* Notes
*
**************************************************************/
void HelloTriangleApplication::createAndFillCommandBuffers()
{
	m_vkCommandBuffers.resize(m_vkSwapchainFrameBuffers.size());
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_vkCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)m_vkCommandBuffers.size();
	if (VK_SUCCESS != vkAllocateCommandBuffers(m_vkDevice, &allocInfo, m_vkCommandBuffers.data()))
	{
		throw std::runtime_error("Could not create command buffers");
	}

	for (size_t i = 0; i < m_vkCommandBuffers.size(); ++i)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		vkBeginCommandBuffer(m_vkCommandBuffers[i], &beginInfo);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_vkRenderPass;
		renderPassInfo.framebuffer = m_vkSwapchainFrameBuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_vkSwapchainExtent;

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.5f, 0.5f, 0.5f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(m_vkCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(m_vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkGraphicsPipeline);
		for (int j = 0; j < m_models.size(); ++j)
		{
			VkBuffer vertexBuffers[] = { m_models[j].getVertexBuffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(m_vkCommandBuffers[i], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(m_vkCommandBuffers[i], m_models[j].getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindDescriptorSets(
				m_vkCommandBuffers[i],
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_vkPipelineLayout,
				0,
				1,
				&m_vkDescriptorSets[j],
				0,
				nullptr);
			vkCmdDrawIndexed(m_vkCommandBuffers[i], m_models[j].getIndicesSize(), 1, 0, 0, 0);
		}
		vkCmdEndRenderPass(m_vkCommandBuffers[i]);

		if (VK_SUCCESS != vkEndCommandBuffer(m_vkCommandBuffers[i]))
		{
			throw std::runtime_error("Failed to record comand buffer.");
		}
	}
}
