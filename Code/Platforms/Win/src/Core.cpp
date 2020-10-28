#include <Core.hpp>

#include <memory>

#include <Renderer.hpp>
#include <Window.hpp>
#include <UI.hpp>
#include <GB.hpp>
#include <EmulationManager.hpp>

Core* Core::mInstance = nullptr;

Core::Core( EmuRender* renderer, EmuWindow* window, EmuUI* ui ) : pRenderer( renderer ), pWindow( window ), pUI( ui )
{
	mInstance = this;

	InitWindows();


	pUI->AddMenuItem( {"Emulator","Stop"}, "Emulator-Stop" );


	pUI->AddMenuItem( {"File","Exit"}, "EXIT" );


	// Force start the emulator
	//pUI->MarkSelectedElement( "Emulator-Start" );
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

bool Core::StartEmulator( EEmulator emulator, const char* path )
{
	if (IsEmulatorRunning())return false;

	pEmulationManager = std::make_unique<EmulationManager>( emulator, path );

	pEmulationManager->WaitTillReady();

	pVisualisation = std::make_unique<Visualisation>( pEmulationManager->GetScreenWidth(), pEmulationManager->GetScreenHeight() );

	return true;
}

bool Core::IsEmulatorRunning()
{
	return pEmulationManager != nullptr;
}

Core* Core::GetInstance()
{
	return mInstance;
}

void GameVisualisation()
{
	ImVec2 windowSize = ImGui::GetWindowSize();
	EmuUI::GetInstance()->DrawScalingImage( 2, 166, 144, windowSize.x, windowSize.y );
}




void Core::InitWindows()
{
	//pUI->RegisterWindow( new EmuUI::Window( "Game", ImGuiWindowFlags_NoCollapse, GameVisualisation, true ) );
}

void Core::UpdateTriggers()
{

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
