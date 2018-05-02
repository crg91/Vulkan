#include "utilities.h"

/**************************************************************
* Description
*		Helper function to create a buffer and buffer memory.
* Returns
*		void
* Notes
*
**************************************************************/
void createBuffer(
	VkDevice vkDevice,
	VkPhysicalDevice vkPhysicalDevice,
	VkDeviceSize size,
	VkBufferUsageFlags usageFlags,
	VkMemoryPropertyFlags properties,
	VkBuffer &buffer,
	VkDeviceMemory &bufferMemory)
{
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usageFlags;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (VK_SUCCESS != vkCreateBuffer(vkDevice, &bufferCreateInfo, nullptr, &buffer))
	{
		throw std::runtime_error("Could not create vertex buffer.");
	}

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(vkDevice, buffer, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = findMemoryType(vkPhysicalDevice,
		memoryRequirements.memoryTypeBits,
		properties);
	if (VK_SUCCESS != vkAllocateMemory(vkDevice, &memoryAllocateInfo, nullptr, &bufferMemory))
	{
		throw std::runtime_error("Could not allocate vertex buffer memory on device.");
	}
	vkBindBufferMemory(vkDevice, buffer, bufferMemory, 0);
}

/**************************************************************
* Description
*		Copies buffers
* Returns
*		void
* Notes
*
**************************************************************/
void copyBuffer(
	VkDevice vkDevice,
	VkCommandPool vkCommandPool,
	VkQueue vkQueue,
	VkBuffer srcBuffer,
	VkBuffer dstBuffer,
	VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(vkDevice, vkCommandPool);
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	endSingleTimeCommands(vkDevice, vkCommandPool, vkQueue, commandBuffer);
}

uint32_t findMemoryType(
	VkPhysicalDevice vkPhysicalDevice,
	int32_t typeFilter,
	VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &memoryProperties);
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
*		Allocates a command buffer and begins it for the user to
*		record commands.
* Returns
*		VkCommandBuffer
* Notes
*
**************************************************************/
VkCommandBuffer beginSingleTimeCommands(
	VkDevice vkDevice,
	VkCommandPool vkCommandPool)
{
	VkCommandBufferAllocateInfo commandBufferAllocInfo = {};
	commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocInfo.commandPool = vkCommandPool;
	commandBufferAllocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	if (VK_SUCCESS != vkAllocateCommandBuffers(vkDevice, &commandBufferAllocInfo, &commandBuffer))
	{
		throw std::runtime_error("Could not create command buffer for memory transfer.");
	}

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	return commandBuffer;
}

/**************************************************************
* Description
*		Ends commands?
* Returns
*		void
* Notes
*
**************************************************************/
void endSingleTimeCommands(
	VkDevice vkDevice,
	VkCommandPool vkCommandPool,
	VkQueue vkQueue,
	VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(vkQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vkQueue);
	vkFreeCommandBuffers(vkDevice, vkCommandPool, 1, &commandBuffer);
}