#define GATEWARE_ENABLE_CORE				// All libraries need this
#define GATEWARE_ENABLE_SYSTEM				// Graphics libs require system level libraries
#define GATEWARE_ENABLE_GRAPHICS			// Enables all Graphics Libraries
#define GATEWARE_ENABLE_MATH				// Enables Gateware math libraries
#define GATEWARE_ENABLE_INPUT				// Enables Gateware input libraries
#define GATEWARE_ENABLE_AUDIO				// Enables Gateware audio libraries

// Ignore some GRAPHICS libraries we aren't going to use
#define GATEWARE_DISABLE_GDIRECTX11SURFACE	// we have another template for this
#define GATEWARE_DISABLE_GDIRECTX12SURFACE	// we have another template for this
#define GATEWARE_DISABLE_GRASTERSURFACE		// we have another template for this
#define GATEWARE_DISABLE_GOPENGLSURFACE		// we have another template for this

// With what we want & what we don't defined we can include the API
#include "../Gateware/Gateware.h"
#include "renderer.h"

// Open some namespaces to compact the code a bit
using namespace GW;
using namespace CORE;
using namespace SYSTEM;
using namespace GRAPHICS;

// Pop a window and use Vulkan to clear to a black screen
int main()
{
	GWindow win;
	GEventResponder msgs;
	GVulkanSurface vulkan;
	GW::INPUT::GInput input;

	if (+win.Create(0, 0, 800, 600, GWindowStyle::WINDOWEDBORDERED))
	{
		win.SetWindowName("Sierra Negron -- Level Renderer -- Vulkan");
		VkClearValue clrAndDepth[2];
		clrAndDepth[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		clrAndDepth[1].depthStencil = { 1.0f, 0u };

		// Handle resize event
		msgs.Create([&](const GW::GEvent& e) 
		{
			GW::SYSTEM::GWindow::Events q;
			if (+e.Read(q) && q == GWindow::Events::RESIZE)
				clrAndDepth[0].color.float32[2] += 0;//0.01f; // disable
		});
		win.Register(msgs);

#ifndef NDEBUG
		const char* debugLayers[] = 
		{
			"VK_LAYER_KHRONOS_validation",				// standard validation layer
			//"VK_LAYER_LUNARG_standard_validation",	// add if not on MacOS
			//"VK_LAYER_RENDERDOC_Capture"				// add this if you have installed RenderDoc
		};
		if (+vulkan.Create(	win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT, 
							sizeof(debugLayers)/sizeof(debugLayers[0]),
							debugLayers, 0, nullptr, 0, nullptr, false))
#else
		if (+vulkan.Create(win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT))
#endif
		{
			Renderer renderer(win, vulkan);		
			while (+win.ProcessWindowEvents())
			{
				if (+vulkan.StartFrame(2, clrAndDepth))
				{
					renderer.UpdateCamera();
					renderer.Render();
					vulkan.EndFrame(true);

					// Change level
					if (GetAsyncKeyState(VK_F1))
						renderer.ChangeLevel();

					// Pause music
					if (GetAsyncKeyState(VK_F2))
						renderer.PauseMusic();

					// Unpause music
					if (GetAsyncKeyState(VK_F3))
						renderer.ResumeMusic();

					// Exit level
					if (GetAsyncKeyState(VK_ESCAPE))
					{
						renderer.CleanUp(); 
						break;
					}
				}
			}
		}
	}
	return 0;
}