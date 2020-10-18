#include <Core.hpp>

#include <memory>

#include <Renderer.hpp>
#include <Window.hpp>
#include <UI.hpp>
#include <GB.hpp>
#include <EmulationManager.hpp>


Core::Core( EmuRender* renderer, EmuWindow* window, EmuUI* ui ) : pRenderer( renderer ), pWindow( window ), pUI( ui )
{
	InitWindows();




	pUI->AddMenuItem( {"Emulator","Start-Local"}, "Emulator-Start-Local" );
	pUI->AddMenuItem( {"Emulator","Start-Windowed"}, "Emulator-Start-Windowed" );
	pUI->AddMenuItem( {"Emulator","Stop"}, "Emulator-Stop" );


	pUI->AddMenuItem( {"File","Exit"}, "EXIT" );
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

	if (pEmulationManager != nullptr && pVisualisation != nullptr)
	{
		pEmulationManager->SyncEmulator( pVisualisation.get() );
	}
}

void GameVisualisation()
{
	ImVec2 size = ImGui::GetWindowSize();
	EmuUI::GetInstance()->DrawScalingImage( 2, 166, 144, size.x, size.y );
}

void Core::InitWindows()
{
	pUI->RegisterWindow( new EmuUI::Window( "Game", ImGuiWindowFlags_NoCollapse, GameVisualisation, true ) );
}

void Core::UpdateTriggers()
{
	if (pUI->IsSelectedElement( "Emulator-Start-Windowed" ))
	{
		if (pEmulationManager == nullptr)
		{
			pEmulationManager = std::make_unique<EmulationManager>( "Games/GB/Pocket.gb" );

			pEmulationManager->WaitTillReady();

			pVisualisation = std::make_unique<Visualisation>( Visualisation::EVisualisationMode::Windowed, 
				pEmulationManager->GetScreenWidth() * 4, pEmulationManager->GetScreenHeight() * 4,
				pEmulationManager->GetScreenWidth(), pEmulationManager->GetScreenHeight() );
		}
		else
		{
			pVisualisation->SetMode( Visualisation::EVisualisationMode::Windowed );
		}
	}
	if (pUI->IsSelectedElement( "Emulator-Start-Local" ))
	{
		if (pEmulationManager == nullptr)
		{
			pEmulationManager = std::make_unique<EmulationManager>( "Games/GB/Pocket.gb" );

			pEmulationManager->WaitTillReady();

			pVisualisation = std::make_unique<Visualisation>( Visualisation::EVisualisationMode::ImGUI,
				pEmulationManager->GetScreenWidth() * 4, pEmulationManager->GetScreenHeight() * 4,
				pEmulationManager->GetScreenWidth(), pEmulationManager->GetScreenHeight() );
		}
		else
		{
			pVisualisation->SetMode( Visualisation::EVisualisationMode::ImGUI );
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
