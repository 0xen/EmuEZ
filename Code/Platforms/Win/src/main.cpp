#include <iostream>

#include <Window.hpp>
#include <Renderer.hpp>
#include <UI.hpp>

#include <examples/imgui_impl_sdl.h>

// Output when the logic steps happen on threads
#define VERBOSE_LOGIC 1

std::unique_ptr<EmuWindow> pWindow;
std::unique_ptr<EmuRender> pRenderer;
std::unique_ptr<EmuUI> pUI;



void Log(const char* text)
{
#if VERBOSE_LOGIC
	std::cout << text << std::endl;
#endif
}

void CommandBufferCallback(VkCommandBuffer& buffer)
{
	pUI->ImGuiCommandBufferCallback(buffer);
}

void Setup()
{
	// Create window
	pWindow = std::make_unique<EmuWindow>("EmuEZ", 1080, 720);
	// Create renderer instance
	pRenderer = std::make_unique<EmuRender>(pWindow.get());
	// Create UI
	pUI = std::make_unique<EmuUI>(pRenderer.get(), pWindow.get());
	// Prepair command buffer callbacks
	pRenderer->RegisterCommandBufferCallback(CommandBufferCallback);
	// Render a initial blank frame
	pRenderer->Render();
	pWindow->OpenWindow();


}

void Close()
{

	pRenderer.reset();
	pWindow.reset();
}

int main(int, char**)
{
	Setup();
	





	EmuWindow::EWindowStatus windowStatus;
	while((windowStatus = pWindow->GetStatus()) != EmuWindow::EWindowStatus::Exiting)
	{

		pUI->StartRender();


		pUI->RenderMainMenuBar();

		pUI->RenderWindows();




		pUI->StopRender();





		pRenderer->Render();
		pWindow->Poll();
	}

	if (windowStatus == EmuWindow::EWindowStatus::Exiting || windowStatus == EmuWindow::EWindowStatus::Open)
	{
		Close();
	}


	system("pause");
	return 0;
}