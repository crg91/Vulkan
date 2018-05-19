#include "model.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include<tiny_obj_loader.h>

#include <unordered_map>


/**************************************************************
* Description
*		Constructor for the class.
* Returns
*		void
* Notes
*
**************************************************************/
Model::Model()
:m_vkVertexBuffer(VK_NULL_HANDLE),
m_vkVertexBufferMemory(VK_NULL_HANDLE)
{
	m_durations.m_xDuration = 0.0;
	m_durations.m_yDuration = 0.0;
	m_durations.m_zDuration = 0.0;
	m_fKeyPressed[0] = false;
	m_fKeyPressed[1] = false;
	m_fKeyPressed[2] = false;
	m_position = glm::vec3(0.0f);
	m_center = glm::vec3(0.0f);
}

/**************************************************************
* Description
*		Descriptor for the class.
* Returns
*		void
* Notes
*
**************************************************************/
Model::~Model()
{
}

/**************************************************************
* Description
*		updates the rotation around x axis based on the time
*		since last key stroke.
* Returns
*		void
* Notes
*
**************************************************************/
void Model::rotateX()
{
	if (m_fKeyPressed[0])
	{
		if (m_fDirectionPositive[0])
		{
			m_durations.m_xDuration += getDuration(m_lastUpdateTime[0]);
		}
		else
		{
			m_durations.m_xDuration -= getDuration(m_lastUpdateTime[0]);
		}
	}
}

/**************************************************************
* Description
*		updates the rotation around y axis based on time since
*		last key stroke.
* Returns
*		void
* Notes
*
**************************************************************/
void Model::rotateY()
{
	if (m_fKeyPressed[1])
	{
		if (m_fDirectionPositive[1])
		{
			m_durations.m_yDuration += getDuration(m_lastUpdateTime[1]);
		}
		else
		{
			m_durations.m_yDuration -= getDuration(m_lastUpdateTime[1]);
		}
	}
}

/**************************************************************
* Description
*		updates the rotation around z axis based on time elapsed
*		since last key stroke.
* Returns
*		void
* Notes
*
**************************************************************/
void Model::rotateZ()
{
	if (m_fKeyPressed[2])
	{
		if (m_fDirectionPositive[2])
		{
			m_durations.m_zDuration += getDuration(m_lastUpdateTime[2]);
		}
		else
		{
			m_durations.m_zDuration -= getDuration(m_lastUpdateTime[2]);
		}
	}
}

/**************************************************************
* Description
*		Returns the rotation matrix based based on the current state.
* Returns
*		void
* Notes
*
**************************************************************/
glm::mat4 Model::getModelMatrix()
{
	rotateX();
	rotateY();
	rotateZ();

	return
		glm::translate(glm::mat4(1.0f), m_position) *
		glm::translate(glm::mat4(1.0f), -m_center) *
		glm::rotate(glm::mat4(1.0f), m_durations.m_xDuration * glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f))*
		glm::rotate(glm::mat4(1.0f), m_durations.m_yDuration * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f))*
		glm::rotate(glm::mat4(1.0f), m_durations.m_zDuration * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f))*
		glm::translate(glm::mat4(1.0f), m_center);
}

/**************************************************************
* Description
*		Gets the duration by looking at current time and last time.
* Returns
*		duration
* Notes
*
**************************************************************/
float Model::getDuration(StdTime &lastTime)
{
	auto currentTime = std::chrono::high_resolution_clock::now();
	float duration = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
	lastTime = currentTime;
	return duration;
}

/**************************************************************
* Description
*		Sets the state of the key for x axis rotation
* Returns
*		void
* Notes
*
**************************************************************/
void Model::setXKeyPressed(bool fKeyPressed)
{
	m_fKeyPressed[0] = fKeyPressed;
	m_lastUpdateTime[0] = std::chrono::high_resolution_clock::now();
}

/**************************************************************
* Description
*		Sets the state of the key for y axis rotation
* Returns
*		void
* Notes
*
**************************************************************/
void Model::setYKeyPressed(bool fKeyPressed)
{
	m_fKeyPressed[1] = fKeyPressed;
	m_lastUpdateTime[1] = std::chrono::high_resolution_clock::now();
}

/**************************************************************
* Description
*		Sets the state of the key for z axis rotation
* Returns
*		void
* Notes
*
**************************************************************/
void Model::setZKeyPressed(bool fKeyPressed)
{
	m_fKeyPressed[2] = fKeyPressed;
	m_lastUpdateTime[2] = std::chrono::high_resolution_clock::now();
}

/**************************************************************
* Description
*		Sets the direction for the x axis rotation
* Returns
*		void
* Notes
*
**************************************************************/
void Model::fXDirectionPositive(bool fPositive)
{
	m_fDirectionPositive[0] = fPositive;
}

/**************************************************************
* Description
*		Sets the direction for the y axis rotation
* Returns
*		void
* Notes
*
**************************************************************/
void Model::fYDirectionPositive(bool fPositive)
{
	m_fDirectionPositive[1] = fPositive;
}

/**************************************************************
* Description
*		Sets the direction for the z axis rotation
* Returns
*		void
* Notes
*
**************************************************************/
void Model::fZDirectionPositive(bool fPositive)
{
	m_fDirectionPositive[2] = fPositive;
}

/**************************************************************
* Description
*		Loads the model from obj file.
* Returns
*		void
* Notes
*
**************************************************************/
void Model::loadModel()
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, m_modelPath.c_str()))
	{
		throw std::runtime_error(err);
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};
	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex = {};
			vertex.m_position = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2] };

			if (index.texcoord_index != -1)
			{
				vertex.m_texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1] };
			}

			if (index.normal_index != -1)
			{
				vertex.m_normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2] };
			}

			vertex.m_color = { 1.0f, 0.0f, 0.0f };
			if (0 == uniqueVertices.count(vertex))
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
				m_vertices.push_back(vertex);
			}
			m_indices.push_back(uniqueVertices[vertex]);
		}
	}
}

/**************************************************************
* Description
*		Creates vertex buffer and memory and copies data to it.
* Returns
*		void
* Notes
*
**************************************************************/
void Model::createVertexBuffer(
	VkDevice vkDevice,
	VkPhysicalDevice vkPhysicalDevice,
	VkCommandPool vkCommandPool,
	VkQueue vkQueue)
{
	uint32_t size = sizeof(m_vertices[0]) * m_vertices.size();
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;
	createBuffer(vkDevice,
		vkPhysicalDevice,
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingMemory);

	void *pData = nullptr;
	vkMapMemory(vkDevice, stagingMemory, 0, size, 0, &pData);
	memcpy(pData, m_vertices.data(), static_cast<size_t>(size));
	vkUnmapMemory(vkDevice, stagingMemory);

	createBuffer(vkDevice,
		vkPhysicalDevice,
		size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_vkVertexBuffer,
		m_vkVertexBufferMemory);

	copyBuffer(vkDevice, vkCommandPool, vkQueue, stagingBuffer, m_vkVertexBuffer, size);
	vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
	vkFreeMemory(vkDevice, stagingMemory, nullptr);
}


/**************************************************************
* Description
*		Creates index buffer, memoory and copies data to it.
* Returns
*		void
* Notes
*
**************************************************************/
void Model::createIndexBuffer(
	VkDevice vkDevice,
	VkPhysicalDevice vkPhysicalDevice,
	VkCommandPool vkCommandPool,
	VkQueue vkQueue)
{
	uint32_t size = sizeof(m_indices[0]) * m_indices.size();
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;
	createBuffer(vkDevice,
		vkPhysicalDevice,
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingMemory);

	void *pData = nullptr;
	vkMapMemory(vkDevice, stagingMemory, 0, size, 0, &pData);
	memcpy(pData, m_indices.data(), static_cast<size_t>(size));
	vkUnmapMemory(vkDevice, stagingMemory);

	createBuffer(vkDevice,
		vkPhysicalDevice,
		size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_vkIndexBuffer,
		m_vkIndexBufferMemory);

	copyBuffer(vkDevice, vkCommandPool, vkQueue, stagingBuffer, m_vkIndexBuffer, size);
	vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
	vkFreeMemory(vkDevice, stagingMemory, nullptr);
}

/**************************************************************
* Description
*		Creates uniform buffer and memory associated with it.
* Returns
*		void
* Notes
*
**************************************************************/
void Model::createUniformBuffer(
	VkDevice vkDevice,
	VkPhysicalDevice vkPhysicalDevice)
{
	uint32_t size = sizeof(UniformBufferObject);
	createBuffer(vkDevice,
		vkPhysicalDevice,
		size,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		m_vkUniformBuffer,
		m_vkUniformBufferMemory);
}

/**************************************************************
* Description
*		Cleans up the objects.
* Returns
*		void
* Notes
*
**************************************************************/
void Model::cleanup(VkDevice vkDevice)
{
	vkDestroyBuffer(vkDevice, m_vkVertexBuffer, nullptr);
	vkFreeMemory(vkDevice, m_vkVertexBufferMemory, nullptr);
	vkDestroyBuffer(vkDevice, m_vkIndexBuffer, nullptr);
	vkFreeMemory(vkDevice, m_vkIndexBufferMemory, nullptr);
	vkDestroyBuffer(vkDevice, m_vkUniformBuffer, nullptr);
	vkFreeMemory(vkDevice, m_vkUniformBufferMemory, nullptr);
}


/**************************************************************
* Description
*		Translates the object by given vector.
* Returns
*		void
* Notes
*
**************************************************************/
void Model::translate(glm::vec3 translationVector)
{
	m_position += translationVector;
}

/**************************************************************
* Description
*		Sets the center for the model. We will use the center to
*		ensure that rotation happens around this axis.
* Returns
*		void
* Notes
*
**************************************************************/
void Model::setCenter(glm::vec3 centerVector)
{
	m_center = centerVector;
}