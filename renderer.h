// Include model class
#include "model.h"

// Creation, Rendering & Cleanup
class Renderer
{
	// Collect the mesh names and their matrices
	struct GameLevelData
	{
		std::vector<H2B::Parser> modelData;				// Every mesh in the level
		std::vector<std::string> modelNames;			// Names of each model
		std::vector<GW::MATH::GMATRIXF> modelMatrices;  // model world matrices
		std::vector<GW::MATH::GVECTORF> pLightPos;		// point light positions in the scene
	};
	GameLevelData					m_levelData = {};

	// proxy handles
	GW::SYSTEM::GWindow				win;
	GW::GRAPHICS::GVulkanSurface	vlk;
	GW::CORE::GEventReceiver		shutdown;
	GW::INPUT::GInput				m_inputProxy;
	GW::INPUT::GController			m_controllerProxy;
	GW::MATH::GMatrix				m_mxMathProxy;
	GW::MATH::GVector				m_vecMathProxy;
	GW::AUDIO::GMusic				m_musicProxy;
	GW::AUDIO::GSound				m_sound;
	GW::AUDIO::GAudio				m_audio;

	// Device and pipeline objects
	VkDevice						m_device			= nullptr;
	VkPipeline						m_pipeline			= nullptr;
	VkPipelineLayout				m_pipelineLayout	= nullptr;

	// Shader modules
	VkShaderModule					m_vertexShader		= nullptr;
	VkShaderModule					m_pixelShader		= nullptr;

	// Models
	std::vector<Model>				m_models;

	// Camera matrices
	GW::MATH::GMATRIXF				m_view;
	GW::MATH::GMATRIXF				m_projection;

	// Used to compute delta time
	XTime							m_timer;

	// Flag for toggling level
	bool m_levelFlag				= false;

	float m_fov, m_ar				= 0.0f;
	unsigned int m_width, m_height	= 0;

public:
	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GVulkanSurface _vlk)
	{
		const char* musicPath = "../Assets/Audio/Dungeon.wav";
		const char* soundPath = "../Assets/Audio/Success.wav";

		// Get Client Dimensions 
		win = _win;
		vlk = _vlk;

		win.GetClientWidth(m_width);
		win.GetClientHeight(m_height);

		// Enable proxies
		m_vecMathProxy.Create();
		m_mxMathProxy.Create();
		m_inputProxy.Create(win);
		m_controllerProxy.Create();
		m_audio.Create();
		m_sound.Create(soundPath, m_audio, 0.005f);		// its very loud!
		m_musicProxy.Create(musicPath, m_audio, 0.005f);

		/* INITIALIZE SCENE DATA */
		InitSceneData(vlk);

		/***************** GEOMETRY INTIALIZATION ****************/
		// Grab the device & physical device so we can allocate some stuff
		VkPhysicalDevice physicalDevice = nullptr;
		vlk.GetDevice((void**)&m_device);
		vlk.GetPhysicalDevice((void**)&physicalDevice);

		// Determine max frames and loop to initialize all buffers
		// We give each frame its own buffer to avoid per-frame resource sharing issues
		unsigned int maxFrames = 0;
		vlk.GetSwapchainImageCount(maxFrames);
		InitGeometry(physicalDevice, maxFrames);

		/***************** SHADER INTIALIZATION ******************/
		InitShaders();

		/***************** PIPELINE INTIALIZATION ****************/
		VkRenderPass renderPass;
		vlk.GetRenderPass((void**)&renderPass);
		InitPipeline(m_width, m_height, renderPass);

		// Play looping background music
		m_musicProxy.Play(true);

		/***************** CLEANUP / SHUTDOWN ********************/
		// GVulkanSurface will inform us when to release any allocated resources
		shutdown.Create(vlk, [&]()
			{
				if (+shutdown.Find(GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES, true))
				{
					CleanUp(); // unlike D3D we must be careful about destroy timing
				}
			});
	}

	void InitSceneData(GW::GRAPHICS::GVulkanSurface _vlk)
	{
		// Determine which level to load
		std::string level;
		if (m_levelFlag == false)
			level = "../GameLevel.txt";
		else
			level = "../GameLevel2.txt";

		// Separate out the per-level models into their own objects
		LoadModels(m_models, level);

		// VIEW MATRIX
		GW::MATH::GVECTORF eye { 0.75f, 2.0f,  3.0f };	// eye position
		GW::MATH::GVECTORF at  {-0.15f, 0.75f, 0.0f };	// where it's looking
		GW::MATH::GVECTORF up  { 0.0f,  1.0f,  0.0f };	// up direction 
		m_mxMathProxy.LookAtLHF(eye, at, up, m_view);	// this performs the inverse operation

		// PROJECTION MATRIX
		_vlk.GetAspectRatio(m_ar);
		m_fov = DegreesToRadians(65);
		m_mxMathProxy.ProjectionVulkanLHF(m_fov, m_ar, 0.1f, 100.0f, m_projection);

		// LIGHTING INFO
		GW::MATH::GVECTORF lightDir		{-1.0f,-1.0f,  2.0f,  0.0f };					// direction wants w = 0
		GW::MATH::GVECTORF lightClr		{ 1.0f, 0.55f, 0.0f,  1.0f };					// orange tint
		GW::MATH::GVECTORF lightAmbient { 0.25f,0.25f, 0.35f, 1.0f };
		GW::MATH::GVECTORF pointColor1  { 1.0f, 0.23f, 0.033f,1.0f };
		GW::MATH::GVECTORF pointColor2	{ 1.0f, 0.5f,  0.1f,  1.0f };

		// Normalize light direction before sending to shaders
		m_vecMathProxy.NormalizeF(lightDir, lightDir);

		// Take the view's position from world space (put it back in world space)
		GW::MATH::GMATRIXF inverseView;
		m_mxMathProxy.InverseF(m_view, inverseView);
		GW::MATH::GVECTORF camPos = inverseView.row4;

		for (int i = 0; i < m_levelData.modelData.size(); i++)
		{
			// Set each model's world matrix and scene data
			m_models[i].m_sceneData.matricies[0]		= m_levelData.modelMatrices[i];

			m_models[i].m_sceneData.sunDirection		= lightDir;
			m_models[i].m_sceneData.sunColor			= lightClr;
			m_models[i].m_sceneData.sunAmbient			= lightAmbient;
			m_models[i].m_sceneData.camPos				= camPos;
			m_models[i].m_sceneData.viewMatrix			= m_view;
			m_models[i].m_sceneData.projMatrix			= m_projection;
			m_models[i].m_sceneData.lightCount			= m_levelData.pLightPos.size(); // set number of lights to size of light vector

			// Point light info
			for (int j = 0; j < m_levelData.pLightPos.size(); j++)
			{
				m_models[i].m_sceneData.pLightPos[j]	= m_levelData.pLightPos[j];  

				if (level == "../GameLevel.txt")
					m_models[i].m_sceneData.pointCol	= pointColor1;
				else 
					m_models[i].m_sceneData.pointCol	= pointColor2;
			}
		}

		// for each model
		for (auto &m : m_models)
		{
			// For each material in the mesh, set the scene data's materials
			for (int i = 0; i < m.m_mesh.materialCount; ++i)
				m.m_sceneData.materials[i] = m.m_mesh.materials[i].attrib;
		}
	}

	void InitGeometry(VkPhysicalDevice _physicalDevice, unsigned int _maxFrames)
	{
		/* INITIALIZE VERTEX BUFFERS, INDEX BUFFERS, AND STORAGE BUFFERS*/
		for (auto& m : m_models)
		{
			m.CreateVertexBuffer(m_device, _physicalDevice);
			m.CreateIndexBuffer(m_device, _physicalDevice);
			m.CreateStorageBuffer(m_device, _physicalDevice, _maxFrames);

			/* ***************** DESCRIPTOR SET ******************* */
			m.InitDescriptorSetLayoutBindingAndCreateInfo(m_device);
			m.InitDescriptorPoolCreateInfo(m_device, _maxFrames);
			m.InitDescriptorSetAllocInfo(m_device, _maxFrames);
			m.WriteDescriptorSet(m_device, _maxFrames);
		}
	}

	void InitShaders()
	{
		std::string vertexShaderSource		= ShaderToString("../VertexShader.hlsl");
		std::string pixelShaderSource		= ShaderToString("../PixelShader.hlsl");

		// Intialize runtime shader compiler HLSL->SPIRV
		shaderc_compiler_t compiler			= shaderc_compiler_initialize();
		shaderc_compile_options_t options	= shaderc_compile_options_initialize();
		shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);
		shaderc_compile_options_set_invert_y(options, false); // enable/disable Y inversion

#ifndef NDEBUG
		shaderc_compile_options_set_generate_debug_info(options);
#endif
		// VERTEX SHADER
		shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
			compiler, vertexShaderSource.c_str(), strlen(vertexShaderSource.c_str()),
			shaderc_vertex_shader, "main.vert", "main", options);

		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Vertex Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;

		GvkHelper::create_shader_module(m_device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &m_vertexShader);

		shaderc_result_release(result); // done

		// PIXEL SHADER
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
	}

	void InitPipeline(unsigned int _width, unsigned int _height, VkRenderPass &_renderPass)
	{
		// Stage Info for vertex/fragment shaders
		VkPipelineShaderStageCreateInfo stage_create_info[2] = {};
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
			{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12 }, // UVW (texture)
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
			0, 0, static_cast<float>(_width), static_cast<float>(_height), 0, 1
		};

		VkRect2D scissor = { {0, 0}, {_width, _height} };
		VkPipelineViewportStateCreateInfo viewport_create_info = {};
		viewport_create_info.sType							= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_create_info.viewportCount					= 1;			// 2 viewports if we want to split screen
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

		// Descriptor pipeline layout
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
		pipeline_create_info.renderPass						= _renderPass;
		pipeline_create_info.subpass						= 0;
		pipeline_create_info.basePipelineHandle				= VK_NULL_HANDLE;
		vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1,
			&pipeline_create_info, nullptr, &m_pipeline);
	}

	void Render()
	{
		// Update specular component and view matrix
		for (int i = 0; i < m_models.size(); i++)
		{
			GW::MATH::GMATRIXF inverseView;
			m_mxMathProxy.InverseF(m_view, inverseView);
			m_models[i].m_sceneData.camPos		= inverseView.row4;
			m_models[i].m_sceneData.viewMatrix	= m_view;
		}

		// Grab the current Vulkan commandBuffer
		unsigned int currentBuffer;
		vlk.GetSwapchainCurrentImage(currentBuffer);
		VkCommandBuffer commandBuffer;
		vlk.GetCommandBuffer(currentBuffer, (void**)&commandBuffer);

		// What is the current client area dimensions?
		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);

		// Setup the pipeline's dynamic settings
		VkViewport viewport =
		{
			0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1
		};

		VkRect2D scissor = { {0, 0}, {width, height} };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

		// Bind and draw each model
		for (auto &m : m_models)
		{
			m.BindBuffers(m_device, m_pipelineLayout, commandBuffer, currentBuffer);
			m.Draw(m_pipelineLayout, commandBuffer);
		}
	}

	void ChangeLevel()
	{
		// Play a sound upon changing the scene
		m_sound.Play();

		// Change the level flag
		m_levelFlag = (false) ? m_levelFlag == true : m_levelFlag == false;
			
		// Finish current level's queue operations
		CleanUp();

		// Clear and re-initialize model data
		m_levelData.modelData.clear();
		m_levelData.modelMatrices.clear();
		m_levelData.modelNames.clear();
		m_models.clear();

		// Re-initialize scene data
		InitSceneData(vlk);
		m_models.resize(m_models.size());

		// Re-initialize geometry
		VkPhysicalDevice physicalDevice = nullptr;
		vlk.GetDevice((void**)&m_device);
		vlk.GetPhysicalDevice((void**)&physicalDevice);
		unsigned int maxFrames = 0;
		vlk.GetSwapchainImageCount(maxFrames);
		InitGeometry(physicalDevice, maxFrames);

		InitShaders();

		VkRenderPass renderPass;
		vlk.GetRenderPass((void**)&renderPass);
		InitPipeline(m_width, m_height, renderPass);
	}

	void UpdateCamera()
	{
		m_timer.Signal();

		// Set view matrix back to world space
		GW::MATH::GMATRIXF viewCopy;
		m_mxMathProxy.InverseF(m_view, viewCopy);

		float delta							= m_timer.Delta();
		float ychange						= 0.0f;				// represents how much we want the 'y' value to change this frame
		const float camSpeed				= 2.0f;				// Represents how far we want the camera to be able to move over one second

		// Vertical input states
		float spacePressed					= 0.0f;
		float lshiftPressed					= 0.0f;				// keyboard
		float rtPressed						= 0.0f;
		float ltPressed						= 0.0f;				// controller

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
		float perFrameSpeed			= camSpeed * delta;
		float xChange				= 0.0f;
		float zChange				= 0.0f;

		// wasd strafing keystates
		float wPressed				= 0.0f;
		float aPressed				= 0.0f;
		float sPressed 				= 0.0f;
		float dPressed				= 0.0f;

		// left stick movement states
		float lStickX				= 0.0f;
		float lStickY				= 0.0f;

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

		float pi					= 3.14159f;
		float thumbSpeed			= pi * delta;
		float totalPitch			= 0.0f;
		float totalYaw				= 0.0f;

		// Right stick states
		float rStickX				= 0.0f;
		float rStickY				= 0.0f;

		// Right controller stick
		m_controllerProxy.GetState(0, G_RX_AXIS, rStickX);
		m_controllerProxy.GetState(0, G_RY_AXIS, rStickY);

		// Mouse deltas
		float mX, mY = 0.0f;

		// Get mouse input
		GW::GReturn res = m_inputProxy.GetMouseDelta(mX, mY);
		if (res == GW::GReturn::SUCCESS && res != GW::GReturn::REDUNDANT || rStickY)
		{
			totalPitch = m_fov * mY / screenHeight + rStickY * -thumbSpeed; // -thumbspeed prevents inverted tilt
			m_mxMathProxy.RotateXLocalF(viewCopy, totalPitch, viewCopy);
		}

		//  Yaw rotation
		if (res == GW::GReturn::SUCCESS && res != GW::GReturn::REDUNDANT || rStickX)
		{
			totalYaw = m_fov * m_ar * mX / screenWidth + rStickX * thumbSpeed;
			m_mxMathProxy.RotateYGlobalF(viewCopy, totalYaw, viewCopy);
		}

		// Set back to view space
		m_mxMathProxy.InverseF(viewCopy, m_view);
	}

	// Runs the parser and populates a vector of Models
	void LoadModels(std::vector<Model>& _models, std::string _gameLevelPath)
	{
		ParseH2B(m_levelData, _gameLevelPath);
		for (int i = 0; i < m_levelData.modelData.size(); i++)
		{
			Model temp;
			temp.m_mesh = m_levelData.modelData[i];
			_models.push_back(temp);
		}
	}

	void PauseMusic() { m_musicProxy.Pause(); }

	void ResumeMusic() { m_musicProxy.Resume(); }

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
	void ParseH2B(GameLevelData& _data, std::string& _filePath)
	{
		H2B::Parser p;
		std::string line				= " ";
		std::string prevName			= " ";
		std::string ignore[2]			= { "<Matrix" , "4x4" };
		GW::MATH::GMATRIXF tempMatrix	= { 0 };
		int ndx							= 0;
		std::vector<std::string> tempNames;

		std::ifstream file{ _filePath, std::ios::in };		// Open file

		if (!file.is_open())
			std::cout << "ParseH2B: Could not open file!\n" << "Path: " << _filePath << std::endl;
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
					strcpy(temp, line.c_str());				// convert line to char*
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

						// Once the last element is filled, push into vector and reset
						if (ndx == 16)
						{
							_data.modelMatrices.push_back(tempMatrix);
							tempMatrix = { 0 };
							ndx = 0;
						}
					} // Walk through Token
				} // For every row in the Matrix
			} // if "MESH"

			if (line == "LIGHT")							// if you found light
			{
				std::getline(file, line);					// Skip name line to get to the matrix
				for (int i = 0; i < 4; i++)					// loop through each row
				{
					char temp[250];
					char* getfloat;

					std::getline(file, line);				// Get the 'i'th row of the matrix
					strcpy(temp, line.c_str());				// convert line to char*
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

						// Once the last element is filled, push the last row into vector and reset
						if (ndx == 16)
						{
							_data.pLightPos.push_back(tempMatrix.row4);
							tempMatrix = { 0 };
							ndx = 0;
						}
					} // Walk through Token
				} // For every row in the Matrix
			} // if "LIGHT"
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
			prevName = _data.modelNames[i];
		}
		// All done!
		file.close();
	}

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