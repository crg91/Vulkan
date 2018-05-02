#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include<iostream>

// Create buffer and its associated memory
// based on input parameters.
//
void createBuffer(
	VkDevice vkDevice,
	VkPhysicalDevice vkPhysicalDevice,
	VkDeviceSize size,
	VkBufferUsageFlags usageFlags,
	VkMemoryPropertyFlags properties,
	VkBuffer &buffer,
	VkDeviceMemory &bufferMemory);

// Copies buffer content.
//
void copyBuffer(
	VkDevice vkDevice,
	VkCommandPool vkCommandPool,
	VkQueue vkQueue,
	VkBuffer srcBuffer,
	VkBuffer dstBuffer,
	VkDeviceSize size);

// Begins a command buffer in the provided
// command pool.
//
VkCommandBuffer beginSingleTimeCommands(
	VkDevice vkDevice,
	VkCommandPool vkCommandPool);

// Submits the command buffer to the queue
// and deletes it.
//
void endSingleTimeCommands(
	VkDevice vkDevice,
	VkCommandPool vkCommandPool,
	VkQueue vkQueue,
	VkCommandBuffer commandBuffer);

// Get the memory type based on the input properties.
//
uint32_t findMemoryType(
	VkPhysicalDevice vkPhysicalDevice,
	int32_t typeFilter,
	VkMemoryPropertyFlags properties);