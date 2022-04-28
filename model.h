#include "shaderc/shaderc.h"	// needed for compiling shaders at runtime
#include "XTime.h"
#include "h2bParser.h"

#ifdef _WIN32					// must use MT platform DLL libraries on windows
#pragma comment(lib, "shaderc_combined.lib") 
#endif

// Organize storage buffer data to send to shaders
#define MAX_SUBMESH_PER_DRAW 1024

// Helper func 
float DegreesToRadians(float _angle)
{
	return _angle * (3.14f / 180.0f);
}

class Model
{
private:
	friend class Renderer;

	// Create struct for shader model data to be passed into the shaders
	struct SHADER_MODEL_DATA
	{
		// Globally shared model data
		GW::MATH::GVECTORF		sunDirection, sunColor, sunAmbient, camPos, pointPos, pointCol;	// light info
		GW::MATH::GMATRIXF		viewMatrix, projMatrix;											// view info

		// Per sub-mesh transformation and material data
		GW::MATH::GMATRIXF		matricies[MAX_SUBMESH_PER_DRAW];								// world space transforms
		H2B::ATTRIBUTES			materials[MAX_SUBMESH_PER_DRAW];								// color/texture of surface
	};
	SHADER_MODEL_DATA			m_sceneData			= { 0 };

	// MODEL SPECIFIC MEMBERS
	H2B::Parser					m_mesh;

	// Vertex/Index buffer handles
	VkBuffer					m_vertexBuffer		= nullptr;
	VkDeviceMemory				m_vertexData		= nullptr;
	VkBuffer					m_indexBuffer		= nullptr;
	VkDeviceMemory				m_indexData			= nullptr;

	// Allocate vectors of vkbuffer/memory for storage buffers
	std::vector<VkBuffer>		m_storageHandle;
	std::vector<VkDeviceMemory> m_storageData;

	// One descriptor set per storage buffer
	std::vector<VkDescriptorSet> m_descriptorSet;

	// Handle to descriptor set layout and descriptor pool
	VkDescriptorSetLayout		m_descriptorLayout	= nullptr;
	VkDescriptorPool			m_descriptorPool	= nullptr;

public:
	// CREATE PER-MODEL BUFFERS
	void CreateVertexBuffer(VkDevice &_device, VkPhysicalDevice &_physicalDevice)
	{
		GvkHelper::create_buffer(
			_physicalDevice,
			_device,
			sizeof(H2B::VERTEX) * (m_mesh.vertexCount),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&m_vertexBuffer, &m_vertexData);
		GvkHelper::write_to_buffer(_device, m_vertexData, m_mesh.vertices.data(), sizeof(H2B::VERTEX) * m_mesh.vertexCount);
	}

	void CreateIndexBuffer(VkDevice &_device, VkPhysicalDevice &_physicalDevice)
	{
		GvkHelper::create_buffer(
			_physicalDevice,
			_device,
			sizeof(unsigned int) * (m_mesh.indexCount),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&m_indexBuffer, &m_indexData);
		GvkHelper::write_to_buffer(_device, m_indexData, m_mesh.indices.data(), sizeof(unsigned int) * (m_mesh.indexCount));
	}

	void CreateStorageBuffer(VkDevice &_device, VkPhysicalDevice &_physicalDevice, unsigned int _maxFrames)
	{
		m_storageHandle.resize(_maxFrames);
		m_storageData.resize(_maxFrames);
		for (int i = 0; i < _maxFrames; i++)
		{
			// Allocate a storage buffer for each frame
			GvkHelper::create_buffer(_physicalDevice, _device, sizeof(SHADER_MODEL_DATA),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&m_storageHandle[i], &m_storageData[i]);

			GvkHelper::write_to_buffer(_device, m_storageData[i], &m_sceneData, sizeof(SHADER_MODEL_DATA));
		}
	}

	// SET UP PER-MODEL DESCRIPTOR SETS FOR THEIR STORAGE BUFFERS
	void InitDescriptorSetLayoutBindingAndCreateInfo(VkDevice &_device)
	{
		VkDescriptorSetLayoutBinding descriptorLayoutBinding = {};
		descriptorLayoutBinding.descriptorCount				= 1;
		descriptorLayoutBinding.descriptorType				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorLayoutBinding.stageFlags					= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		descriptorLayoutBinding.binding						= 0;
		descriptorLayoutBinding.pImmutableSamplers			= nullptr;

		// Tells vulkan how many bindings we have and where they are
		VkDescriptorSetLayoutCreateInfo descriptorCreateInfo = {};
		descriptorCreateInfo.sType							= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorCreateInfo.flags							= 0;
		descriptorCreateInfo.bindingCount					= 1;
		descriptorCreateInfo.pBindings						= &descriptorLayoutBinding;
		descriptorCreateInfo.pNext							= nullptr;
		vkCreateDescriptorSetLayout(_device, &descriptorCreateInfo,
			nullptr, &m_descriptorLayout);
	}

	void InitDescriptorPoolCreateInfo(VkDevice &_device, unsigned int _maxFrames)
	{
		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
		descriptorPoolCreateInfo.sType						= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		VkDescriptorPoolSize dpSize							= { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, _maxFrames };		// {type, count}
		descriptorPoolCreateInfo.poolSizeCount				= 1;														// num of elements in pPoolSizes
		descriptorPoolCreateInfo.pPoolSizes					= &dpSize;													// pointer to array of PoolSize structs, containing a type and number of descriptors
		descriptorPoolCreateInfo.maxSets					= _maxFrames;												// max number of descriptor sets that CAN be allocated from the pool
		descriptorPoolCreateInfo.flags						= 0;														// no special flags
		descriptorPoolCreateInfo.pNext						= nullptr;
		vkCreateDescriptorPool(_device, &descriptorPoolCreateInfo, nullptr, &m_descriptorPool); // successful
	}

	void InitDescriptorSetAllocInfo(VkDevice &_device, unsigned int _maxFrames)
	{
		VkDescriptorSetAllocateInfo descriptorAllocInfo = {};
		descriptorAllocInfo.sType							= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorAllocInfo.descriptorSetCount				= 1;
		descriptorAllocInfo.pSetLayouts						= &m_descriptorLayout;
		descriptorAllocInfo.descriptorPool					= m_descriptorPool;
		descriptorAllocInfo.pNext							= nullptr;
		m_descriptorSet.resize(_maxFrames);
		for (int i = 0; i < _maxFrames; ++i)
		{
			vkAllocateDescriptorSets(_device, &descriptorAllocInfo, &m_descriptorSet[i]);
		}
	}

	void WriteDescriptorSet(VkDevice &_device, unsigned int _maxFrames)
	{
		/* Link descriptor set to storage buffer */
		VkWriteDescriptorSet writeDescriptorSet = {};
		writeDescriptorSet.sType							= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.descriptorCount					= 1;
		writeDescriptorSet.dstArrayElement					= 0;
		writeDescriptorSet.dstBinding						= 0;
		writeDescriptorSet.descriptorType					= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		for (int i = 0; i < _maxFrames; ++i) // got rid of the validation error for invalid descriptor (there were too many because we're only using one right now)
		{
			writeDescriptorSet.dstSet						= m_descriptorSet[i];
			VkDescriptorBufferInfo dbufferInfo				= { m_storageHandle[i], 0, VK_WHOLE_SIZE }; // whole size uses range from the offset to the end of the buffer
			writeDescriptorSet.pBufferInfo					= &dbufferInfo;
			vkUpdateDescriptorSets(_device, 1, &writeDescriptorSet, 0, nullptr);
		}
	}

	// BIND VERTEX/INDEX/STORAGE BUFFERS
	void BindBuffers(VkDevice _device, VkPipelineLayout _pipelineLayout, VkCommandBuffer _commandBuffer, unsigned int _currentBuffer)
	{
		VkDeviceSize offsets[] = { 0 };
		// Bind vertex/index buffers
		vkCmdBindVertexBuffers(_commandBuffer, 0, 1, &m_vertexBuffer, offsets);
		vkCmdBindIndexBuffer(_commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		// Connect descriptor set to command buffer
		vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			_pipelineLayout, 0, 1, &m_descriptorSet[_currentBuffer], 0, nullptr);

		// update storage buffer
		GvkHelper::write_to_buffer(_device, m_storageData[_currentBuffer], &m_sceneData, sizeof(Model::SHADER_MODEL_DATA));
	}

	void Draw(VkPipelineLayout &_pipelineLayout, VkCommandBuffer &_commandBuffer)
	{
		// for each submesh
		for (int i = 0; i < m_mesh.meshes.size(); ++i)
		{
			// send each mesh's material index to the shaders right before calling draw
			vkCmdPushConstants(_commandBuffer, _pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, sizeof(uint32_t), &m_mesh.meshes[i].materialIndex);

			// Draw each submesh by their indexCounts and offsets
			for (int j = 0; j < m_mesh.meshes.size(); ++j) 
					vkCmdDrawIndexed(_commandBuffer, m_mesh.meshes[j].drawInfo.indexCount, 1, m_mesh.meshes[j].drawInfo.indexOffset, 0, 0);	// SHOULD draw split by submeshes
				
			//vkCmdDrawIndexed(_commandBuffer, m_mesh.indexCount, 1, 0, 0, 0);															// draws whole mesh
		}
	}

	// Clean up
	void CleanUpModelData(VkDevice& _device)
	{
		vkDestroyBuffer(_device, m_indexBuffer, nullptr);
		vkFreeMemory(_device, m_indexData, nullptr);
		vkDestroyBuffer(_device, m_vertexBuffer, nullptr);
		vkFreeMemory(_device, m_vertexData, nullptr);

		// Free the storage buffers
		for (int i = 0; i < m_storageData.size(); ++i)
		{
			vkDestroyBuffer(_device, m_storageHandle[i], nullptr);
			vkFreeMemory(_device, m_storageData[i], nullptr);
		}
		m_storageData.clear();
		m_storageHandle.clear();

		// Clean up descriptor set
		vkDestroyDescriptorSetLayout(_device, m_descriptorLayout, nullptr);

		// Clean up descriptor pool
		vkDestroyDescriptorPool(_device, m_descriptorPool, nullptr);
	}
};