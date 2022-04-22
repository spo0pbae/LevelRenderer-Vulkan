// Include model class
#include "model.h"

// Creation, Rendering & Cleanup
class Renderer
{
	// proxy handles
	GW::SYSTEM::GWindow				win;
	GW::GRAPHICS::GVulkanSurface	vlk;
	GW::CORE::GEventReceiver		shutdown;
	GW::INPUT::GInput				m_inputProxy;
	GW::INPUT::GController			m_controllerProxy;
	GW::MATH::GMatrix				m_mxMathProxy;
	GW::MATH::GVector				m_vecMathProxy;

	VkDevice						m_device			= nullptr;
	VkPipeline						m_pipeline			= nullptr;
	VkPipelineLayout				m_pipelineLayout	= nullptr;

	// Shader modules
	VkShaderModule					m_vertexShader		= nullptr;
	VkShaderModule					m_pixelShader		= nullptr;

	// Models
	std::vector<Model>				m_models;

	// Create matrices
	GW::MATH::GMATRIXF				m_world;
	GW::MATH::GMATRIXF				m_view;
	GW::MATH::GMATRIXF				m_projection;

	XTime m_timer;

	float m_fov = 0.0f;
	float m_ar	= 0.0f;

public:
	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GVulkanSurface _vlk)
	{
		// Get Client Dimensions 
		win = _win;
		vlk = _vlk;
		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);

		// Enable proxies
		m_inputProxy.Create(win);
		m_controllerProxy.Create();

		// Separate out the models so we can give them their own buffers
		Model temp;
		temp.LoadModels(m_models);

		/* INITIALIZE SCENE DATA (for each model)*/

		// create proxies
		m_vecMathProxy.Create();
		m_mxMathProxy.Create();

		// WORLD MATRIX
		m_world = GW::MATH::GIdentityMatrixF;

		// VIEW MATRIX
		GW::MATH::GVECTORF eye{ 0.75f, 0.25f,-3.0f };	// eye position
		GW::MATH::GVECTORF at { 0.15f, 0.75f, 0.0f };	// where it's looking
		GW::MATH::GVECTORF up { 0.0f,  1.0f,  0.0f };	// how high up 
		m_mxMathProxy.LookAtLHF(eye, at, up, m_view);	// this performs the inverse operation

		// PROJECTION MATRIX
		_vlk.GetAspectRatio(m_ar);
		m_fov = DegreesToRadians(65);
		m_mxMathProxy.ProjectionVulkanLHF(m_fov, m_ar, 0.1f, 100.0f, m_projection);	// left handed projection matrix(fov, ar, z near, z far, outMx)

		/* Lighting info */
		GW::MATH::GVECTORF lightDir		{ -1.0f,-1.0f,  2.0f,  0.0f };					// adding 0 here because direction wants w = 0
		GW::MATH::GVECTORF lightClr		{ 0.9f,  0.9f,  1.0f,  1.0f };					// mostly white with bluish tinge
		GW::MATH::GVECTORF lightAmbient	{ 0.25f, 0.25f, 0.35f, 1.0f };

		// NORMALIZE LIGHT DIRECTION
		m_vecMathProxy.NormalizeF(lightDir, lightDir);

		// take the view's position from world space (put it back in world space)
		GW::MATH::GMATRIXF inverseView;
		m_mxMathProxy.InverseF(m_view, inverseView);
		GW::MATH::GVECTORF camPos = inverseView.row4;

		for (int i = 0; i < temp.m_levelData.modelData.size(); i++)
		{
			// copy the level data
			m_models[i].m_levelData.modelData		= temp.m_levelData.modelData;
			m_models[i].m_levelData.modelNames		= temp.m_levelData.modelNames;
			m_models[i].m_levelData.modelMatrices	= temp.m_levelData.modelMatrices;

			// set each model's world matrix and scene data
			m_models[i].m_sceneData.matricies[0]	= temp.m_levelData.modelMatrices[i];

			m_models[i].m_sceneData.sunDirection	= lightDir;
			m_models[i].m_sceneData.sunColor		= lightClr;
			m_models[i].m_sceneData.sunAmbient		= lightAmbient;
			m_models[i].m_sceneData.camPos			= camPos;
			m_models[i].m_sceneData.viewMatrix		= m_view;
			m_models[i].m_sceneData.projMatrix		= m_projection;
		}

		// for each material in the model, set to the materials attributes, which is first element in material struct
		for (auto &m : m_models)
		{
			for (int i = 0; i < m.m_mesh.materialCount; ++i)
				m.m_sceneData.materials[i] = m.m_mesh.materials[i].attrib;
		}

		/***************** GEOMETRY INTIALIZATION ******************/
		/* Grab the device & physical device so we can allocate some stuff */
		VkPhysicalDevice physicalDevice = nullptr;
		vlk.GetDevice((void**)&m_device);
		vlk.GetPhysicalDevice((void**)&physicalDevice);

		/* Determine max frames and loop to initialize all buffers
		 * We give each frame its own buffer to avoid per-frame resource sharing issues */
		unsigned int maxFrames = 0;
		vlk.GetSwapchainImageCount(maxFrames);

		/* INITIALIZE VERTEX BUFFERS, INDEX BUFFERS, AND STORAGE BUFFERS*/
		for (auto &m : m_models)
		{
			m.CreateVertexBuffer(m_device, physicalDevice);
			m.CreateIndexBuffer(m_device, physicalDevice);
			m.CreateStorageBuffer(m_device, physicalDevice, maxFrames);
		
			/* ***************** DESCRIPTOR SET ******************* */
			m.InitDescriptorSetLayoutBindingAndCreateInfo(m_device);
			m.InitDescriptorPoolCreateInfo(m_device, maxFrames);
			m.InitDescriptorSetAllocInfo(m_device, maxFrames);
			m.WriteDescriptorSet(m_device, maxFrames);
		}

		/***************** SHADER INTIALIZATION ******************/
		std::string vertexShaderSource = ShaderToString("../VertexShader.hlsl");
		std::string pixelShaderSource  = ShaderToString("../PixelShader.hlsl");

		/* Intialize runtime shader compiler HLSL->SPIRV */
		shaderc_compiler_t compiler		  = shaderc_compiler_initialize();
		shaderc_compile_options_t options = shaderc_compile_options_initialize();
		shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);
		shaderc_compile_options_set_invert_y(options, false); // enable/disable Y inversion

#ifndef NDEBUG
		shaderc_compile_options_set_generate_debug_info(options);
#endif
		// Create Vertex Shader
		shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
			compiler, vertexShaderSource.c_str(), strlen(vertexShaderSource.c_str()),
			shaderc_vertex_shader, "main.vert", "main", options);

		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Vertex Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;

		GvkHelper::create_shader_module(m_device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &m_vertexShader);

		shaderc_result_release(result); // done

		// Create Pixel Shader
		result = shaderc_compile_into_spv( // compile
			compiler, pixelShaderSource.c_str(), strlen(pixelShaderSource.c_str()),
			shaderc_fragment_shader, "main.frag", "main", options);

		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Pixel Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;

		GvkHelper::create_shader_module(m_device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &m_pixelShader);

		shaderc_result_release(result); // done

		// Free runtime shader compiler resources
		shaderc_compile_options_release(options);
		shaderc_compiler_release(compiler);

		/***************** PIPELINE INTIALIZATION ****************/
		// Create Pipeline & Layout (Thanks Tiny!)
		VkRenderPass renderPass;
		vlk.GetRenderPass((void**)&renderPass);
		VkPipelineShaderStageCreateInfo stage_create_info[2] = {};

		/* Create Stage Info for vertex/fragment shaders */
		stage_create_info[0].sType							= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[0].stage							= VK_SHADER_STAGE_VERTEX_BIT;
		stage_create_info[0].module							= m_vertexShader;//models[0].m_vertexShader;
		stage_create_info[0].pName							= "main";

		stage_create_info[1].sType							= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[1].stage							= VK_SHADER_STAGE_FRAGMENT_BIT;
		stage_create_info[1].module							= m_pixelShader;//models[0].m_pixelShader;
		stage_create_info[1].pName							= "main";

		// Assembly State
		VkPipelineInputAssemblyStateCreateInfo assembly_create_info = {};
		assembly_create_info.sType							= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assembly_create_info.topology						= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		assembly_create_info.primitiveRestartEnable			= false;

		// Vertex Input State
		VkVertexInputBindingDescription vertex_binding_description = {};
		vertex_binding_description.binding					= 0;
		vertex_binding_description.stride					= sizeof(H2B::VERTEX); // 36 bytes
		vertex_binding_description.inputRate				= VK_VERTEX_INPUT_RATE_VERTEX;

		// Attributes of the Mesh
		VkVertexInputAttributeDescription vertex_attribute_description[3] =
		{
			// Location, binding, format, offset
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0  }, // pos
			{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12 }, // UVW (texture)	-- 12 byte offset (3 floats, 4 bytes each)
			{ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, 24 }  // Normal
		};

		VkPipelineVertexInputStateCreateInfo input_vertex_info = {};
		input_vertex_info.sType								= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		input_vertex_info.vertexBindingDescriptionCount		= 1;
		input_vertex_info.pVertexBindingDescriptions		= &vertex_binding_description;
		input_vertex_info.vertexAttributeDescriptionCount	= 3;
		input_vertex_info.pVertexAttributeDescriptions		= vertex_attribute_description;

		// Viewport State (we still need to set this up even though we will overwrite the values)
		VkViewport viewport =
		{
			0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1
		};

		VkRect2D scissor = { {0, 0}, {width, height} };
		VkPipelineViewportStateCreateInfo viewport_create_info = {};
		viewport_create_info.sType							= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_create_info.viewportCount					= 1;
		viewport_create_info.pViewports						= &viewport;
		viewport_create_info.scissorCount					= 1;
		viewport_create_info.pScissors						= &scissor;

		// Rasterizer State
		VkPipelineRasterizationStateCreateInfo rasterization_create_info = {};
		rasterization_create_info.sType						= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization_create_info.rasterizerDiscardEnable	= VK_FALSE;
		rasterization_create_info.polygonMode				= VK_POLYGON_MODE_FILL;
		rasterization_create_info.lineWidth					= 1.0f;
		rasterization_create_info.cullMode					= VK_CULL_MODE_BACK_BIT;
		rasterization_create_info.frontFace					= VK_FRONT_FACE_CLOCKWISE;
		rasterization_create_info.depthClampEnable			= VK_FALSE;
		rasterization_create_info.depthBiasEnable			= VK_FALSE;
		rasterization_create_info.depthBiasClamp			= 0.0f;
		rasterization_create_info.depthBiasConstantFactor	= 0.0f;
		rasterization_create_info.depthBiasSlopeFactor		= 0.0f;

		// Multisampling State (Anti-ailiasing)
		VkPipelineMultisampleStateCreateInfo multisample_create_info = {};
		multisample_create_info.sType						= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample_create_info.sampleShadingEnable			= VK_FALSE;
		multisample_create_info.rasterizationSamples		= VK_SAMPLE_COUNT_1_BIT;
		multisample_create_info.minSampleShading			= 1.0f;
		multisample_create_info.pSampleMask					= VK_NULL_HANDLE;
		multisample_create_info.alphaToCoverageEnable		= VK_FALSE;
		multisample_create_info.alphaToOneEnable			= VK_FALSE;

		// Depth-Stencil State
		VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = {};
		depth_stencil_create_info.sType						= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil_create_info.depthTestEnable			= VK_TRUE;
		depth_stencil_create_info.depthWriteEnable			= VK_TRUE;
		depth_stencil_create_info.depthCompareOp			= VK_COMPARE_OP_LESS;
		depth_stencil_create_info.depthBoundsTestEnable		= VK_FALSE;
		depth_stencil_create_info.minDepthBounds			= 0.0f;
		depth_stencil_create_info.maxDepthBounds			= 1.0f;
		depth_stencil_create_info.stencilTestEnable			= VK_FALSE;

		// Color Blending Attachment & State
		VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
		color_blend_attachment_state.colorWriteMask			= 0xF;
		color_blend_attachment_state.blendEnable			= VK_FALSE;
		color_blend_attachment_state.srcColorBlendFactor	= VK_BLEND_FACTOR_SRC_COLOR;
		color_blend_attachment_state.dstColorBlendFactor	= VK_BLEND_FACTOR_DST_COLOR;
		color_blend_attachment_state.colorBlendOp			= VK_BLEND_OP_ADD;
		color_blend_attachment_state.srcAlphaBlendFactor	= VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachment_state.dstAlphaBlendFactor	= VK_BLEND_FACTOR_DST_ALPHA;
		color_blend_attachment_state.alphaBlendOp			= VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo color_blend_create_info = {};
		color_blend_create_info.sType						= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blend_create_info.logicOpEnable				= VK_FALSE;
		color_blend_create_info.logicOp						= VK_LOGIC_OP_COPY;
		color_blend_create_info.attachmentCount				= 1;
		color_blend_create_info.pAttachments				= &color_blend_attachment_state;
		color_blend_create_info.blendConstants[0]			= 0.0f;
		color_blend_create_info.blendConstants[1]			= 0.0f;
		color_blend_create_info.blendConstants[2]			= 0.0f;
		color_blend_create_info.blendConstants[3]			= 0.0f;

		// Dynamic State 
		VkDynamicState dynamic_state[2] =
		{
			// By setting these we do not need to re-create the pipeline on Resize
			VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamic_create_info = {};
		dynamic_create_info.sType							= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_create_info.dynamicStateCount				= 2;
		dynamic_create_info.pDynamicStates					= dynamic_state;

		// descriptor set moved to loop // 

		// Push constants let us send small amounts of data to the shaders, in this case our material/materialID from the fs logo meshes
		VkPushConstantRange pushConstant = {};
		pushConstant.offset									= 0;
		pushConstant.size									= sizeof(uint32_t); // needs to be at least 128 bytes
		pushConstant.stageFlags								= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		/* Descriptor pipeline layout */
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		pipeline_layout_create_info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_create_info.setLayoutCount			= 1;
		pipeline_layout_create_info.pSetLayouts				= &m_models[0].m_descriptorLayout;//VK_NULL_HANDLE;
		pipeline_layout_create_info.pushConstantRangeCount	= 1;					// number of pushconstant ranges
		pipeline_layout_create_info.pPushConstantRanges		= &pushConstant;
		vkCreatePipelineLayout(m_device, &pipeline_layout_create_info,
			nullptr, &m_pipelineLayout);

		// Pipeline State... (FINALLY) 
		VkGraphicsPipelineCreateInfo pipeline_create_info = {};
		pipeline_create_info.sType							= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_create_info.stageCount						= 2;
		pipeline_create_info.pStages						= stage_create_info;
		pipeline_create_info.pInputAssemblyState			= &assembly_create_info;
		pipeline_create_info.pVertexInputState				= &input_vertex_info;
		pipeline_create_info.pViewportState					= &viewport_create_info;
		pipeline_create_info.pRasterizationState			= &rasterization_create_info;
		pipeline_create_info.pMultisampleState				= &multisample_create_info;
		pipeline_create_info.pDepthStencilState				= &depth_stencil_create_info;
		pipeline_create_info.pColorBlendState				= &color_blend_create_info;
		pipeline_create_info.pDynamicState					= &dynamic_create_info;
		pipeline_create_info.layout							= m_pipelineLayout;
		pipeline_create_info.renderPass						= renderPass;
		pipeline_create_info.subpass						= 0;
		pipeline_create_info.basePipelineHandle				= VK_NULL_HANDLE;
		vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1,
			&pipeline_create_info, nullptr, &m_pipeline);

		/***************** CLEANUP / SHUTDOWN ******************/
		// GVulkanSurface will inform us when to release any allocated resources
		shutdown.Create(vlk, [&]()
			{
				if (+shutdown.Find(GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES, true))
				{
					CleanUp(); // unlike D3D we must be careful about destroy timing
				}
			});
	}

	void Render()
	{
		for (auto &m : m_models)
		m.m_sceneData.viewMatrix = m_view;

		/* Grab the current Vulkan commandBuffer */
		unsigned int currentBuffer;
		vlk.GetSwapchainCurrentImage(currentBuffer);
		VkCommandBuffer commandBuffer;
		vlk.GetCommandBuffer(currentBuffer, (void**)&commandBuffer);

		/* what is the current client area dimensions? */
		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);

		/* setup the pipeline's dynamic settings */
		VkViewport viewport =
		{
			0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1
		};

		VkRect2D scissor = { {0, 0}, {width, height} };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

		/* BIND BUFFERS */
		// TEMPORARY BINDING (since the renderer wont go into my bind function) ///
		VkDeviceSize offsets[] = { 0 };

		// Bind vertex/index buffers & descriptor
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_models[0].m_vertexBuffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, m_models[0].m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipelineLayout, 0, 1, &m_models[0].m_descriptorSet[currentBuffer], 0, nullptr);
		GvkHelper::write_to_buffer(m_device, m_models[0].m_storageData[currentBuffer], &m_models[0].m_sceneData, sizeof(Model::SHADER_MODEL_DATA));
		 ///////////////////////////////////////////////////////////////////////////

		/* DRAW */
		// we want each model to draw it's mesh
		//for (auto &m : models)
		//{
		//	m.BindBuffers(device, pipelineLayout, commandBuffer, currentBuffer, offsets);
		//	m.Draw(pipelineLayout, commandBuffer);
		//}

		// Testing drawing a single model from the model vector
			//models[0].BindBuffers(device, pipelineLayout, commandBuffer, currentBuffer);
		m_models[0].Draw(m_pipelineLayout, commandBuffer);
	}

	void UpdateCamera()
	{
		m_timer.Signal();
		float delta = m_timer.Delta();

		// Set view matrix back to world space
		GW::MATH::GMATRIXF viewCopy;
		m_mxMathProxy.InverseF(m_view, viewCopy);

		float ychange			= 0.0f;		// represents how much we want the y value to change this frame
		const float camSpeed	= 2.5f;		// Represents how far we want the camera to be able to move over one second

		// Vertical input states
		float spacePressed		= 0.0f;
		float lshiftPressed		= 0.0f;
		float rtPressed			= 0.0f;
		float ltPressed			= 0.0f;

		// Get vertical input states
		m_inputProxy.GetState(G_KEY_SPACE, spacePressed);
		m_inputProxy.GetState(G_KEY_LEFTSHIFT, lshiftPressed);
		m_controllerProxy.GetState(0, G_RIGHT_TRIGGER_AXIS, rtPressed);
		m_controllerProxy.GetState(0, G_LEFT_TRIGGER_AXIS, ltPressed);

		if (spacePressed || lshiftPressed || rtPressed || ltPressed)
		{
			// make the camera move on the Y
			ychange = spacePressed - lshiftPressed + rtPressed - ltPressed;
			viewCopy.row4.y += ychange * camSpeed * delta;
		}

		// Handle WASD movement
		GW::MATH::GVECTORF translateMx;
		float perFrameSpeed = camSpeed * delta;
		float xChange	= 0.0f;
		float zChange	= 0.0f;

		// wasd strafing keystates
		float wPressed	= 0.0f;
		float aPressed	= 0.0f;
		float sPressed	= 0.0f;
		float dPressed	= 0.0f;

		// left stick movement states
		float lStickX	= 0.0f;
		float lStickY	= 0.0f;

		// WASD
		m_inputProxy.GetState(G_KEY_W, wPressed);
		m_inputProxy.GetState(G_KEY_A, aPressed);
		m_inputProxy.GetState(G_KEY_S, sPressed);
		m_inputProxy.GetState(G_KEY_D, dPressed);

		// Left controller stick
		m_controllerProxy.GetState(0, G_LX_AXIS, lStickX);
		m_controllerProxy.GetState(0, G_LY_AXIS, lStickY);

		if (wPressed || aPressed || sPressed || dPressed || lStickX || lStickY)
		{
			zChange = wPressed - sPressed + lStickY;
			xChange = dPressed - aPressed + lStickX;

			translateMx = { xChange * perFrameSpeed, 0.0f, zChange * perFrameSpeed };
			m_mxMathProxy.TranslateLocalF(viewCopy, translateMx, viewCopy);
		}

		// Handle rotation (Yaw/Pitch)
		unsigned int screenWidth, screenHeight;
		win.GetClientWidth(screenWidth);
		win.GetClientHeight(screenHeight);

		//GW::MATH::GVECTORF pitchVec;		//may need to be a matrix to be multiplied instead
		//GW::MATH::GMATRIXF pitchMx;

		float pi			= 3.14159f;
		float thumbSpeed	= pi * delta;
		float totalPitch	= 0.0f; 		// pitch is essentially vertical tilt (X rotation)
		float totalYaw		= 0.0f;

		// Right stick states
		float rStickX		= 0.0f;
		float rStickY		= 0.0f;

		// right controller stick
		m_controllerProxy.GetState(0, G_RX_AXIS, rStickX);
		m_controllerProxy.GetState(0, G_RY_AXIS, rStickY);

		// Mouse deltas
		float mX = 0.0f;
		float mY = 0.0f;

		// Get mouse input
		GW::GReturn res = m_inputProxy.GetMouseDelta(mX, mY);
		if (res == GW::GReturn::SUCCESS && res != GW::GReturn::REDUNDANT || rStickY)
		{
			totalPitch = m_fov * mY / screenHeight + rStickY * -thumbSpeed; // -thumbspeed prevents inverted tilt
			m_mxMathProxy.RotateXLocalF(viewCopy, totalPitch, viewCopy);
		}
		//  yaw rotation
		if (res == GW::GReturn::SUCCESS && res != GW::GReturn::REDUNDANT || rStickX)
		{
			totalYaw = m_fov * m_ar * mX / screenWidth + rStickX * thumbSpeed;
			m_mxMathProxy.RotateYGlobalF(viewCopy, totalYaw, viewCopy);
		}

		// Set back to view space
		m_mxMathProxy.InverseF(viewCopy, m_view);
	}

	std::string ShaderToString(const char* _shaderFilePath)
	{
		std::string output;
		unsigned int stringLength = 0;
		GW::SYSTEM::GFile file; file.Create();
		file.GetFileSize(_shaderFilePath, stringLength);
		if (stringLength && +file.OpenBinaryRead(_shaderFilePath))
		{
			output.resize(stringLength);
			file.Read(&output[0], stringLength);
		}
		else
			std::cout << "ERROR: Shader Source File \"" << _shaderFilePath << "\" Not Found!" << std::endl;
		return output;
	}

private:
	void CleanUp()
	{
		// wait till everything has completed
		vkDeviceWaitIdle(m_device);

		// Clean up shaders
		vkDestroyShaderModule(m_device, m_vertexShader, nullptr);
		vkDestroyShaderModule(m_device, m_pixelShader, nullptr);

		// Clean up vertex/index buffers, etc.
		for (auto & m : m_models)
			m.CleanUpModelData(m_device);

		// Clean up pipeline
		vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
		vkDestroyPipeline(m_device, m_pipeline, nullptr);
	}
};