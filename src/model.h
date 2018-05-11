#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include<iostream>
#include<chrono>

typedef std::chrono::time_point<std::chrono::steady_clock> StdTime;

#include "utilities.h"
#include<array>
#include<vector>

// We keep this structure to keep track of durations for
// rotations in each axis.
// TODO : Consider using glm for this.
//
struct DurationForRotation
{
	float m_xDuration;
	float m_yDuration;
	float m_zDuration;
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

struct UniformBufferObject
{
	glm::mat4 m_model;
	glm::mat4 m_view;
	glm::mat4 m_proj;
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


// Keeps track of the state of a model.
//
class Model
{
public:
	Model();
	~Model();
	void rotateX();
	void rotateY();
	void rotateZ();
	float getDuration(StdTime &lastTime);
	glm::mat4 getModelMatrix();
	void setXKeyPressed(bool fKeyPressed);
	void fXDirectionPositive(bool fPositive);
	void setYKeyPressed(bool fKeyPressed);
	void fYDirectionPositive(bool fPositive);
	void setZKeyPressed(bool fKeyPressed);
	void fZDirectionPositive(bool fPositive);
	void loadModel();
	void createVertexBuffer(VkDevice vkDevice, VkPhysicalDevice vkPhysicalDevice, VkCommandPool vkCommandPool, VkQueue vkQueue);
	void createIndexBuffer(VkDevice vkDevice, VkPhysicalDevice vkPhysicalDevice, VkCommandPool vkCommandPool, VkQueue vkQueue);
	void createUniformBuffer(VkDevice vkDevice, VkPhysicalDevice vkPhysicalDevice);
	void cleanup(VkDevice vkDevice);
	VkBuffer getVertexBuffer() { return m_vkVertexBuffer; }
	VkBuffer getIndexBuffer() { return m_vkIndexBuffer; }
	VkBuffer getUniformBuffer() { return m_vkUniformBuffer; }
	VkDeviceMemory getUniformBufferMemory() { return m_vkUniformBufferMemory; }
	uint32_t getIndicesSize() { return static_cast<uint32_t>(m_indices.size()); }
	void setModelPath(std::string modelPath) { m_modelPath = modelPath; }
private:
	DurationForRotation m_durations;
	StdTime m_lastUpdateTime[3];
	bool m_fKeyPressed[3];
	bool m_fDirectionPositive[3];
	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;
	VkBuffer m_vkVertexBuffer;
	VkDeviceMemory m_vkVertexBufferMemory;
	VkBuffer m_vkIndexBuffer;
	VkDeviceMemory m_vkIndexBufferMemory;
	VkBuffer m_vkUniformBuffer;
	VkDeviceMemory m_vkUniformBufferMemory;
	std::string m_modelPath;
};

