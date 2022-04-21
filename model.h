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

	// Collect the mesh names and their matrices
	struct GameLevelData
	{
		//std::vector<unsigned int> uniqueMeshOffsets;	// offsets in the meshData vector...
		//std::vector<H2B::Parser> uniqueMeshes;		// all unique meshes
		std::vector<H2B::Parser> modelData;				// every mesh in the level
		std::vector<std::string> modelNames;
		std::vector<GW::MATH::GMATRIXF> modelMatrices;
	};

	// Create struct for shader model data to be passed into the shaders
	struct SHADER_MODEL_DATA
	{
		// Globally shared model data
		GW::MATH::GVECTORF		sunDirection, sunColor, sunAmbient, camPos;			// light info
		GW::MATH::GMATRIXF		viewMatrix, projMatrix;								// view info

		// Per sub-mesh transformation and material data
		GW::MATH::GMATRIXF		matricies[MAX_SUBMESH_PER_DRAW]; // world space transforms
		H2B::ATTRIBUTES			materials[MAX_SUBMESH_PER_DRAW]; // color/texture of surface
	};
	SHADER_MODEL_DATA			m_sceneData			= { 0 };
	GameLevelData				m_levelData			= {};

	// MODEL SPECIFIC MEMBERS
	H2B::Parser					m_mesh;
	//VkShaderModule				m_vertexShader		= nullptr;
	//VkShaderModule				m_pixelShader		= nullptr;

	// Create Vertex/Index buffer handles
	VkBuffer					m_vertexBuffer		= nullptr;
	VkDeviceMemory				m_vertexData		= nullptr;
	VkBuffer					m_indexBuffer		= nullptr;
	VkDeviceMemory				m_indexData			= nullptr;

	// Allocate vectors of vkbuffer/memory for storage buffers
	std::vector<VkBuffer>		m_storageHandle;
	std::vector<VkDeviceMemory> m_storageData;

	// Extend the descriptor set into a vector ; we want one per storage buffer
	std::vector<VkDescriptorSet> m_descriptorSet;

	// Handle to descriptor set layout and descriptor pool
	VkDescriptorSetLayout		m_descriptorLayout	= nullptr;
	VkDescriptorPool			m_descriptorPool	= nullptr;

	// Create matrices (and proxies)
	GW::MATH::GMatrix			m_matMathProxy;
	GW::MATH::GVector			m_lightProxy;
	GW::MATH::GMATRIXF			m_world;
	GW::MATH::GMATRIXF			m_view;
	GW::MATH::GMATRIXF			m_projection;

	float m_fov = 0.0f;
	float m_ar = 0.0f;

public:

	// Adding model arg to initialize the scene data for each model
	void InitSceneData(GW::GRAPHICS::GVulkanSurface& _vlk, Model &_model)
	{
		// create proxies
		_model.m_lightProxy.Create();
		_model.m_matMathProxy.Create();

		// WORLD MATRIX
		_model.m_world = GW::MATH::GIdentityMatrixF;

		// VIEW MATRIX
		GW::MATH::GVECTORF eye { 0.75f, 0.25f,-3.0f };	// eye position
		GW::MATH::GVECTORF at  { 0.15f, 0.75f, 0.0f };	// where it's looking
		GW::MATH::GVECTORF up  { 0.0f,  1.0f,  0.0f };	// how high up 
		_model.m_matMathProxy.LookAtLHF(eye, at, up, _model.m_view);  // this performs the inverse operation

		// PROJECTION MATRIX
		_vlk.GetAspectRatio(_model.m_ar);
		_model.m_fov = DegreesToRadians(65);
		_model.m_matMathProxy.ProjectionVulkanLHF(_model.m_fov, _model.m_ar, 0.1f, 100.0f, _model.m_projection);	// left handed projection matrix(fov, ar, z near, z far, outMx)

		/* Lighting info */
		GW::MATH::GVECTORF lightDir		{ -1.0f, -1.0f,  2.0f,  0.0f };					// adding 0 here because direction wants w = 0
		GW::MATH::GVECTORF lightClr		{ 0.9f,  0.9f,  1.0f,  1.0f };					// mostly white with bluish tinge
		GW::MATH::GVECTORF lightAmbient	{ 0.25f, 0.25f, 0.35f, 1.0f };

		_model.m_sceneData.sunDirection	= lightDir;
		_model.m_sceneData.sunColor		= lightClr;
		_model.m_sceneData.sunAmbient	= lightAmbient;
		_model.m_sceneData.viewMatrix	= _model.m_view;
		_model.m_sceneData.projMatrix	= _model.m_projection;

		// for each model, set the model's world matrices
		for (int i = 0; i < m_levelData.modelData.size(); ++i)
		{
			_model.m_sceneData.matricies[i] = _model.m_levelData.modelMatrices[i];
		}
		// for each material in the model, set to the materials attributes, which is first element in material struct
		for (int i = 0; i < _model.m_mesh.materialCount; ++i)
		{
			_model.m_sceneData.materials[i] = _model.m_mesh.materials[i].attrib;
		}
	}

	// Create per-mesh buffers
	void CreateVertexBuffer(VkDevice &_device, VkPhysicalDevice &_physicalDevice)
	{
		GvkHelper::create_buffer(
			_physicalDevice,
			_device,
			sizeof(H2B::VERTEX) * m_mesh.vertexCount,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&m_vertexBuffer, &m_vertexData);
		GvkHelper::write_to_buffer(_device, m_vertexData, m_mesh.vertices.data(), sizeof(H2B::VERTEX) * m_mesh.vertexCount);
	}

	void CreateIndexBuffer(VkDevice &_device, VkPhysicalDevice &_physicalDevice)
	{
		// Create Index buffer
		GvkHelper::create_buffer(
			_physicalDevice,
			_device,
			sizeof(unsigned int) * m_mesh.indexCount,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&m_indexBuffer, &m_indexData);
		GvkHelper::write_to_buffer(_device, m_indexData, m_mesh.indices.data(), sizeof(unsigned int) * m_mesh.indexCount);
	}

	void CreateStorageBuffer(VkDevice &_device, VkPhysicalDevice &_physicalDevice, unsigned int _maxFrames)
	{
		/* CREATE STORAGE BUFFER */
		m_storageHandle.resize(_maxFrames);
		m_storageData.resize(_maxFrames);
		for (int i = 0; i < _maxFrames; i++)
		{
			// Allocate a storage buffer for each frame
			GvkHelper::create_buffer(_physicalDevice, _device, sizeof(SHADER_MODEL_DATA),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&m_storageHandle[i], &m_storageData[i]); // changed from 0

			GvkHelper::write_to_buffer(_device, m_storageData[i], &m_sceneData, sizeof(SHADER_MODEL_DATA));
		}
	}

	/*/ Initialize shaders */
//	void InitShaders(VkDevice &_device)
//	{
//		std::string vertexShaderSource		= ShaderToString("../VertexShader.hlsl");
//		std::string pixelShaderSource		= ShaderToString("../PixelShader.hlsl");
//
//		/* Intialize runtime shader compiler HLSL->SPIRV */
//		shaderc_compiler_t compiler			= shaderc_compiler_initialize();
//		shaderc_compile_options_t options	= shaderc_compile_options_initialize();
//		shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);
//		shaderc_compile_options_set_invert_y(options, false); // enable/disable Y inversion
//
//#ifndef NDEBUG
//		shaderc_compile_options_set_generate_debug_info(options);
//#endif
//		// Create Vertex Shader
//		shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
//			compiler, vertexShaderSource.c_str(), strlen(vertexShaderSource.c_str()),
//			shaderc_vertex_shader, "main.vert", "main", options);
//
//		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
//			std::cout << "Vertex Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
//
//		GvkHelper::create_shader_module(_device, shaderc_result_get_length(result), // load into Vulkan
//			(char*)shaderc_result_get_bytes(result), &m_vertexShader);
//
//		shaderc_result_release(result); // done
//
//		// Create Pixel Shader
//		result = shaderc_compile_into_spv( // compile
//			compiler, pixelShaderSource.c_str(), strlen(pixelShaderSource.c_str()),
//			shaderc_fragment_shader, "main.frag", "main", options);
//
//		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
//			std::cout << "Pixel Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
//
//		GvkHelper::create_shader_module(_device, shaderc_result_get_length(result), // load into Vulkan
//			(char*)shaderc_result_get_bytes(result), &m_pixelShader);
//
//		shaderc_result_release(result); // done
//
//		// Free runtime shader compiler resources
//		shaderc_compile_options_release(options);
//		shaderc_compiler_release(compiler);
//	}

	// Set up descriptor for the shaders. These supply the shaders with external data, in this case the storage buffer
	void InitDescriptorSetLayoutBindingAndCreateInfo(VkDevice &_device)
	{
		VkDescriptorSetLayoutBinding descriptorLayoutBinding = {};
		descriptorLayoutBinding.descriptorCount				= 1;															// only one descriptor for now
		descriptorLayoutBinding.descriptorType				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;							// used for storage buffers
		descriptorLayoutBinding.stageFlags					= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;	// For both shaders
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
		VkDescriptorPoolSize dpSize							= { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, _maxFrames };		// 4f changed from 1 // {type, count}
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

	// Link descriptor set to storage buffer
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

	// Bind vertex/index/storage buffers
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

	// currently drawing a single mesh
	void Draw(VkPipelineLayout& _pipelineLayout, VkCommandBuffer& _commandBuffer)
	{
		// each model's mesh
		for (int i = 0; i < 2; i++)//m_mesh.meshes.size(); i++)
		{
			// send each mesh's material index to the shaders right before calling draw
			vkCmdPushConstants(_commandBuffer, _pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, sizeof(uint32_t), &m_mesh.meshes[i].materialIndex);

			vkCmdDrawIndexed(_commandBuffer, m_mesh.indexCount, 1, m_mesh.meshes[i].drawInfo.indexOffset, 0, 0);
		}
	}

	/*/ Read shader from file */
	//std::string ShaderToString(const char* _shaderFilePath)
	//{
	//	std::string output;
	//	unsigned int stringLength = 0;
	//	GW::SYSTEM::GFile file; file.Create();
	//	file.GetFileSize(_shaderFilePath, stringLength);
	//	if (stringLength && +file.OpenBinaryRead(_shaderFilePath))
	//	{
	//		output.resize(stringLength);
	//		file.Read(&output[0], stringLength);
	//	}
	//	else
	//		std::cout << "ERROR: Shader Source File \"" << _shaderFilePath << "\" Not Found!" << std::endl;
	//	return output;
	//}

	// Clean up
	void CleanUpModelData(VkDevice& _device)
	{
		vkDestroyBuffer(_device, m_indexBuffer, nullptr);
		vkFreeMemory(_device, m_indexData, nullptr);
		vkDestroyBuffer(_device, m_vertexBuffer, nullptr);
		vkFreeMemory(_device, m_vertexData, nullptr);

		/*/ Clean up shaders */
		//vkDestroyShaderModule(_device, m_vertexShader, nullptr);
		//vkDestroyShaderModule(_device, m_pixelShader, nullptr);

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

	// Runs the parser and populates a vector of models, giving them their own shaders/buffers/etc.
	// this is so we can group all of them together and then bind/draw them individually in Render()
	// IN: reference to a vector of models to be filled
	void LoadModels(std::vector<Model> &_models)
	{
		ParseH2B(m_levelData, std::string("../FoxTest.txt"));		// Test level
		for (int i = 0; i < m_levelData.modelData.size(); i++)
		{
			Model temp;
			temp.m_mesh = m_levelData.modelData[i];
			_models.push_back(temp);
		}
	}

	private: 
		void ParseH2B(GameLevelData& _data, std::string& _filePath)
		{
			H2B::Parser p;
			std::string line = " ";
			std::string prevName = " ";
			std::string ignore[2] = { "<Matrix" , "4x4" };
			GW::MATH::GMATRIXF tempMatrix = { 0 };
			int ndx = 0;
			std::vector<std::string> tempNames;

			std::ifstream file{ _filePath, std::ios::in };		// open file

			if (!file.is_open())
				std::cout << "Could not open file!\n";
			else
			{
				// GET NAMES
				while (!file.eof())
				{
					std::getline(file, line);
					if (line == "MESH")							// If we find the mesh
					{
						std::getline(file, line, '.');			// Get the next line to retrieve the mesh name
						tempNames.push_back(line);
						std::getline(file, line);
					}
				}
			}
			file.clear();
			file.seekg(0);
			// GET MATRIX DATA
			while (!file.eof())									// enter an infinite loop (exit if nothing to read)
			{
				std::getline(file, line);						// read current line of text up to the new line
				if (line == "MESH")								// check if you found mesh
				{
					std::getline(file, line);					// Skip name line to get to the matrix
					for (int i = 0; i < 4; i++)					// loop through each row
					{
						char temp[250];
						char* getfloat;

						std::getline(file, line);				// Get the 'i'th row of the matrix
						strcpy(temp, line.c_str());				// convert to char*
						getfloat = strtok(temp, " (,)");		// Get first token in the row

						while (getfloat != NULL)				// walk through the rest of the tokens in the row
						{
							if (getfloat != ignore[0] && getfloat != ignore[1])
							{
								float stof = atof(getfloat);
								tempMatrix.data[ndx] = stof;
								ndx++; // only increment ndx if we put something in it
							}
							getfloat = strtok(NULL, " (,)>");	// Get next token

							// once the last element is filled, push into vector and reset
							if (ndx == 16)
							{
								_data.modelMatrices.push_back(tempMatrix);
								tempMatrix = { 0 };
								ndx = 0;
							}
						} // Walk through Token
					} // For every row in the Matrix
				} // if "MESH"
			} // !eof

			// store actual names in mesh vector
			for (int i = 0; i < tempNames.size(); i++)
			{
				char* path = nullptr;
				char tempPath[75];

				_data.modelNames.push_back(tempNames[i]);

				// check h2b path
				std::string temp = "../Assets/";
				temp.append(_data.modelNames[i]);
				temp.append(".h2b");

				strcpy(tempPath, temp.c_str());
				path = tempPath;

				p.Parse(path);
				_data.modelData.push_back(p);

				// Also fill a vector with unique meshes... just in case?
				//if (_data.meshNames[i] != prevName)
				//{
				//	_data.uniqueMeshes.push_back(p);
				//	_data.uniqueMeshOffsets.push_back(i);
				//}
				prevName = _data.modelNames[i];
			}
			// All done!
			file.close();
		}
};