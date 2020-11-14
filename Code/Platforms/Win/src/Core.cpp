#include <Core.hpp>

#include <sstream>

#include <filesystem>
#include <memory>
#include <locale>

#include <Renderer.hpp>
#include <Window.hpp>
#include <UI.hpp>
#include <GB.hpp>
#include <EmulationManager.hpp>

Core* Core::mInstance = nullptr;

void CoreInputEvent( ConsoleKeys key, bool pressed )
{
	Core::GetInstance( )->InputEvent( key,pressed );
}

Core::Core( EmuRender* renderer, EmuWindow* window, EmuUI* ui ) : pRenderer( renderer ), pWindow( window ), pUI( ui )
{
	mInstance = this;

	InitWindows();

	LoadConfig();
	// Make default save game folder
	std::filesystem::create_directory( ".\\Saves" );
	std::filesystem::create_directory( ".\\SaveStates" );

	pUI->AddMenuItem( {"Emulator","Stop"}, "Emulator-Stop" );
	pUI->AddMenuItem( {"Emulator","Save State"}, "Emulator-Save-State" );
	pUI->AddMenuItem( {"Emulator","Load State"}, "Emulator-Load-State" );
	pUI->AddMenuItem( {"Emulator","Gameboy","SkipBIOS"}, "Emulator-Gameboy-SkipBIOS" );

	pUI->AddMenuItem( {"File","Exit"}, "EXIT" );

	pWindow->RegisterInputEventCallback( EmuWindow::EInputEventSubsystem::Core, CoreInputEvent );

	ScanFolder( "./Games/GB/Games" );
	//ScanFolder( ".\\Games\\GB\\Tests\\mooneye\\acceptance\\timer" );
	//ScanFolder( ".\\Games\\GB\\Tests\\mooneye\\acceptance\\interrupts" );
	//ScanFolder( ".\\Games\\GB\\Tests\\AntonioND\\timers" );

	//ScanFolder( ".\\Games\\GB\\Tests\\mooneye\\emulator-only\\mbc1" );
	//ScanFolder( ".\\Games\\GB\\Tests\\blargs\\interrupt_time" );
	//ScanFolder( ".\\Games\\GB\\Tests\\blargs\\cpu_instrs" );
	//ScanFolder( ".\\Games\\GB\\Tests\\blargs\\instr_timing" );
	//ScanFolder( ".\\Games\\GB\\Tests\\blargs\\mem_timing" );
	//ScanFolder( ".\\Games\\GB\\Tests\\blargs\\mem_timing-2" );
	//ScanFolder( ".\\Games\\GB\\Tests\\blargs\\oam_bug" );



	//mKeyMappings[EView::Emulator][EInputType::JoyButton].push_back( {ConsoleKeys::A, 0} );
	//mKeyMappings[EView::Emulator][EInputType::JoyButton].push_back( {ConsoleKeys::B, 1} );
	//mKeyMappings[EView::Emulator][EInputType::JoyButton].push_back( {ConsoleKeys::START, 7} );
	//mKeyMappings[EView::Emulator][EInputType::JoyButton].push_back( {ConsoleKeys::SELECT, 6} );


	//mKeyMappings[EView::Emulator][EInputType::JoyHat].push_back( {ConsoleKeys::UP, 1} );
	//mKeyMappings[EView::Emulator][EInputType::JoyHat].push_back( {ConsoleKeys::RIGHT, 2} );
	//mKeyMappings[EView::Emulator][EInputType::JoyHat].push_back( {ConsoleKeys::DOWN, 4} );
	//mKeyMappings[EView::Emulator][EInputType::JoyHat].push_back( {ConsoleKeys::LEFT, 8} );


	//mKeyMappings[EView::Emulator][EInputType::JoyAxis].push_back( {ConsoleKeys::UP, 1, -10000} );
	//mKeyMappings[EView::Emulator][EInputType::JoyAxis].push_back( {ConsoleKeys::RIGHT, 0, 10000} );
	//mKeyMappings[EView::Emulator][EInputType::JoyAxis].push_back( {ConsoleKeys::DOWN, 1, 10000} );
	//mKeyMappings[EView::Emulator][EInputType::JoyAxis].push_back( {ConsoleKeys::LEFT, 0, -10000} );


	//mKeyMappings[EView::Emulator][EInputType::Keyboard].push_back( {ConsoleKeys::UP, SDL_SCANCODE_W} );
	//mKeyMappings[EView::Emulator][EInputType::Keyboard].push_back( {ConsoleKeys::RIGHT, SDL_SCANCODE_D} );
	//mKeyMappings[EView::Emulator][EInputType::Keyboard].push_back( {ConsoleKeys::DOWN, SDL_SCANCODE_S} );
	//mKeyMappings[EView::Emulator][EInputType::Keyboard].push_back( {ConsoleKeys::LEFT, SDL_SCANCODE_A} );


}

Core::~Core()
{
	pVisualisation.reset();
	if (pEmulationManager)
	{
		pEmulationManager->Stop();
		pEmulationManager.reset();
	}
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

	pEmulationManager = std::make_unique<EmulationManager>( game, pWindow );

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
	if (!std::filesystem::exists( path ))return;
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

	Save( emuNode );

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

		Load( emuNode );

		pUI->Load( emuNode );

		EmulationManager::Load( emuNode );

	}
}

Core* Core::GetInstance()
{
	return mInstance;
}

std::map<Core::EView, std::map<Core::EInputType, std::vector<Core::KeyInstance*>>>& Core::GetKeyMappings( )
{
	return mKeyMappings;
}

std::map<Core::EView, std::map<ConsoleKeys, std::vector<Core::KeyInstance*>>>& Core::GetKeyBindings( )
{
	return mKeyBindingLookup;
}

void Core::AddKeyBinding( KeyInstance key )
{
	mKeyInstances.push_back( key );
}

void Core::RemoveKeyBinding( KeyInstance* key )
{
	for ( int i = 0; i < mKeyInstances.size( ); i++ )
	{
		if ( &mKeyInstances[i] == key )
		{
			mKeyInstances.erase( mKeyInstances.begin( ) + i );
			return;
		}
	}
}

void Core::RebuildKeyMappings( )
{
	mKeyMappings.clear( );
	mKeyBindingLookup.clear( );

	for ( auto& it : mKeyInstances )
	{
		mKeyMappings[it.view][it.type].push_back( &it );

		mKeyBindingLookup[it.view][it.key].push_back( &it );
	}
}

void Core::InputEvent( ConsoleKeys key, bool pressed )
{
	switch ( key )
	{
	case ConsoleKeys::LOAD_STATE:
	{
		if( pressed ) pEmulationManager->RequestAction( EEmulatorFlags::LoadState );
		break;
	}
	case ConsoleKeys::SAVE_STATE:
	{
		if ( pressed ) pEmulationManager->RequestAction( EEmulatorFlags::SaveState );
		break;
	}
	}
	
}

void Core::Save( pugi::xml_node& node )
{
	pugi::xml_node& inputNode = node.append_child( "Input" );

	for ( auto& it : mKeyInstances )
	{
		pugi::xml_node& iNode = inputNode.append_child( "Type" );
		iNode.append_attribute( "View" ).set_value( (int) it.view );
		iNode.append_attribute( "Type" ).set_value( (int) it.type );
		iNode.append_attribute( "Index" ).set_value( (int) it.index );
		iNode.append_attribute( "Key" ).set_value( (int) it.key );
		iNode.append_attribute( "StartRange" ).set_value( (int) it.startRange );
	}

}

void Core::Load( pugi::xml_node& node )
{
	pugi::xml_node& inputNode = node.child( "Input" );
	for ( pugi::xml_node& typeNode : inputNode.children( "Type" ) )
	{
		mKeyInstances.push_back(
			{
				(EView) typeNode.attribute( "View" ).as_int( 0 ),
				(EInputType) typeNode.attribute( "Type" ).as_int( 0 ),
				(ConsoleKeys) typeNode.attribute( "Key" ).as_int( 0 ),
				typeNode.attribute( "Index" ).as_int( 0 ),
				typeNode.attribute( "StartRange" ).as_int( 0 )
			}
		);
	}
	RebuildKeyMappings( );


}

void Core::InitWindows()
{

}

void Core::UpdateTriggers()
{

	if ( pUI->IsSelectedElement( "Emulator-Save-State" ) )
	{
		if ( pEmulationManager != nullptr )
		{
			pEmulationManager->RequestAction( EEmulatorFlags::SaveState );
		}
	}

	if ( pUI->IsSelectedElement( "Emulator-Load-State" ) )
	{
		if ( pEmulationManager != nullptr )
		{
			pEmulationManager->RequestAction( EEmulatorFlags::LoadState );
		}
	}

	if ( pUI->IsSelectedElement( "Emulator-Stop" ) )
	{
		if ( pEmulationManager != nullptr )
		{
			pEmulationManager->Stop( );
			pEmulationManager.reset( );
			pVisualisation->ClearScreen( );
			pVisualisation.reset( );
		}
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
