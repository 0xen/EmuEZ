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

	LoadConfig();

	pUI->AddMenuItem( {"Emulator","Stop"}, "Emulator-Stop" );
	pUI->AddMenuItem( {"Emulator","Gameboy","SkipBIOS"}, "Emulator-Gameboy-SkipBIOS" );

	pUI->AddMenuItem( {"File","Exit"}, "EXIT" );

	//ScanFolder( "./Games/GB/Games" );
	//ScanFolder( ".\\Games\\GB\\Tests\\mooneye\\acceptance\\timer" );
	//ScanFolder( ".\\Games\\GB\\Tests\\AntonioND\\timers" );
	

	//ScanFolder( ".\\Games\\GB\\Tests\\blargs\\cpu_instrs" );
	ScanFolder( ".\\Games\\GB\\Tests\\blargs\\mem_timing" );
	//ScanFolder( ".\\Games\\GB\\Tests\\blargs\\mem_timing-2" );
}

Core::~Core()
{
	pVisualisation.reset();
	pEmulationManager.reset();
	SaveConfig();
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

	if (extension == ".gb"|| extension == ".gbc")
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

void Core::SaveConfig()
{
	pugi::xml_document doc;

	pugi::xml_node emuNode = doc.append_child( "EmuEZ" );

	pUI->Save( emuNode );

	EmulationManager::Save( emuNode );

	doc.save_file("Config.xml");
}

void Core::LoadConfig()
{
	pugi::xml_document doc;

	pugi::xml_parse_result result = doc.load_file( "Config.xml" );

	if (result.status != pugi::xml_parse_status::status_file_not_found)
	{
		pugi::xml_node& emuNode = doc.child( "EmuEZ" );

		pUI->Load( emuNode );

		EmulationManager::Load( emuNode );

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

	if (pUI->IsSelectedElement( "Emulator-Gameboy-SkipBIOS" ))
	{
		EmulationManager::GetGameboyConfig().mSkipBIOS = !EmulationManager::GetGameboyConfig().mSkipBIOS;
	}


	if (pUI->IsSelectedElement( "EXIT" ))
	{
		pWindow->CloseWindow();
	}
}
