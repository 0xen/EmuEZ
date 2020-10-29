#include <Core.hpp>


#include <filesystem>
#include <memory>
#include <locale>

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

	ScanFolder( "./Games/GB/" );
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

bool Core::StartEmulator( EGame game )
{
	if (IsEmulatorRunning())return false;

	pEmulationManager = std::make_unique<EmulationManager>( game );

	pEmulationManager->WaitTillReady();

	pVisualisation = std::make_unique<Visualisation>( pEmulationManager->GetScreenWidth(), pEmulationManager->GetScreenHeight() );

	return true;
}

bool Core::IsEmulatorRunning()
{
	return pEmulationManager != nullptr;
}

std::vector<EGame>& Core::GetGames()
{
	return mGames;
}

void Core::AddGame( const char* path )
{
	std::string extension = std::filesystem::path( path ).extension().string();

	for (auto& c : extension)
	{
		c = tolower( c );
	}

	EEmulator system;

	if (extension == ".gb")
	{
		system = EEmulator::GB;
	}
	else
	{
		return;
	}



	std::string fileName = std::filesystem::path( path ).replace_extension("").filename().string();




	mGames.push_back( {system, path, fileName.c_str()} );
}

void Core::ScanFolder( const char* path )
{
	for (std::filesystem::recursive_directory_iterator end, dir( path );
		dir != end; ++dir)
	{
		AddGame( dir->path().string().c_str() );
	}
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
