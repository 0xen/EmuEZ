#include <Core.hpp>

#include <memory>

#include <Renderer.hpp>
#include <Window.hpp>
#include <UI.hpp>
#include <GB.hpp>
#include <EmulationManager.hpp>

#define IMGUI_DEFINE_MATH_OPERATORS 1

Core::Core( EmuRender* renderer, EmuWindow* window, EmuUI* ui ) : pRenderer( renderer ), pWindow( window ), pUI( ui )
{
	InitWindows();




	pUI->AddMenuItem( {"Emulator","Start"}, "Emulator-Start" );
	pUI->AddMenuItem( {"Emulator","Stop"}, "Emulator-Stop" );


	pUI->AddMenuItem( {"File","Exit"}, "EXIT" );


	// Force start the emulator
	pUI->MarkSelectedElement( "Emulator-Start" );
}

Core::~Core()
{
	pVisualisation.reset();
	pEmulationManager.reset();
}

void Core::Update()
{
	pUI->StartRender();
	pUI->RenderMainMenuBar();
	pUI->RenderWindows();
	pUI->StopRender();
	pRenderer->Render();
	pWindow->Poll();

	UpdateTriggers();

	pUI->ResetSelectedElements();

	if (pEmulationManager != nullptr && pVisualisation != nullptr)
	{
		pEmulationManager->SyncEmulator( pVisualisation.get() );
	}
}

void GameVisualisation()
{
	ImVec2 windowSize = ImGui::GetWindowSize();
	EmuUI::GetInstance()->DrawScalingImage( 2, 166, 144, windowSize.x, windowSize.y );
}




void Core::InitWindows()
{
	pUI->RegisterWindow( new EmuUI::Window( "Game", ImGuiWindowFlags_NoCollapse, GameVisualisation, true ) );
}

void Core::UpdateTriggers()
{
	if (pUI->IsSelectedElement( "Emulator-Start" ))
	{
		if (pEmulationManager == nullptr)
		{
			pEmulationManager = std::make_unique<EmulationManager>( EEmulator::PSX, "Games/GB/Pocket.gb" );

			pEmulationManager->WaitTillReady();

			pVisualisation = std::make_unique<Visualisation>( pEmulationManager->GetScreenWidth(), pEmulationManager->GetScreenHeight() );
		}
	}
	

	if (pUI->IsSelectedElement( "Emulator-Stop" ))
	{
		pEmulationManager->Stop();
		pEmulationManager.reset();
		pVisualisation->ClearScreen();
		pVisualisation.reset();
	}


	if (pUI->IsSelectedElement( "EXIT" ))
	{
		pWindow->CloseWindow();
	}
}
