#include <EmulationManager.hpp>

#include <Visualisation.hpp>
#include <Window.hpp>
#include <Core.hpp>

#include <GB.hpp>
#include <PSX.hpp>

EmulationManager::GameboyConfig EmulationManager::mGameboy;
EmulationManager* EmulationManager::mInstance = nullptr;


void GameWindowInputEvent( SDL_Event& event )
{
	EmulationManager::GetInstance( )->GameInputEvent( event );
}


EmulationManager::EmulationManager( EGame game, EmuWindow* window ) : mGame( game ), pWindow( window )
{
	mInstance = this;
	mStatus = EEmulatorStatus::Stopped;
	pWindow->RegisterInputEventCallback( EmuWindow::EInputEventSubsystem::Game, GameWindowInputEvent );
	switch (game.emulator)
	{
		case EEmulator::GB:
			mThread = std::thread( &EmulationManager::EmulationLoop<EmuGB>, this );
			break;
		case EEmulator::PSX:
			mThread = std::thread( &EmulationManager::EmulationLoop<EmuPSX>, this );
			break;
	}
}

EmulationManager::~EmulationManager()
{
	mInstance = nullptr;
	pWindow->UnregisterInputEventCallback( EmuWindow::EInputEventSubsystem::Game );
}

void EmulationManager::SyncEmulator( Visualisation* visualisation )
{

	// Lock the mutex and check to see if the emulator has finished the frame, if it has sync the data to the main thread
	{
		std::unique_lock<std::mutex> lock( mMutex.mutex );
		if (mMutex.ready)
		{
			mMutex.ready = false;
			//std::cout << "syncing data" << std::endl;

			visualisation->SetPixels( mEmuScreenBuffer, mEmuScreenBufferSize );


			lock.unlock();
			mMutex.condition.notify_one();
		}
		//std::cout << "rendering" << std::endl;
	}


}

unsigned int EmulationManager::GetScreenWidth()
{
	return mScreenWidth;
}

unsigned int EmulationManager::GetScreenHeight()
{
	return mScreenHeight;
}

void EmulationManager::WaitTillReady()
{
	{
		std::unique_lock<std::mutex> lock( mMutex.mutex );
		while (mStatus == EEmulatorStatus::Stopped)
		{
			mMutex.condition.wait( lock );
		};
	}
}

void EmulationManager::Stop()
{
	{
		std::unique_lock<std::mutex> lock( mMutex.mutex ); 

		while (mStatus != EEmulatorStatus::Stopped)
		{
			if (mMutex.ready)
			{
				mMutex.ready = false;
				mStatus = EEmulatorStatus::StopRequested;
				mMutex.condition.notify_one();
			}
			mMutex.condition.wait( lock );
		};

		mThread.join();
	}
}

void EmulationManager::Save( pugi::xml_node& node )
{
	pugi::xml_node& emulatorNode = node.append_child( "Emulators" );

	{ // Game Boy
		pugi::xml_node& gameboyNode = emulatorNode.append_child( "Gameboy" );

		gameboyNode.append_child( "SkipBIOS" ).append_attribute( "value" ).set_value( mGameboy.mSkipBIOS );
	}

}

void EmulationManager::Load( pugi::xml_node& node )
{
	pugi::xml_node& emulatorNode = node.child( "Emulators" );

	{ // Game Boy
		pugi::xml_node& gameboyNode = emulatorNode.child( "Gameboy" );

		mGameboy.mSkipBIOS = gameboyNode.child( "SkipBIOS" ).attribute( "value" ).as_bool( false );
	}

}

EmulationManager::GameboyConfig& EmulationManager::GetGameboyConfig()
{
	return mGameboy;
}

void EmulationManager::ButtonPress( ConsoleKeys key, bool pressed )
{
	std::unique_lock<std::mutex> lock( mQueuedKeysMutex );
	mQueuedKeys.push_back( {key,pressed} );
}

void EmulationManager::GameInputEvent( SDL_Event& event )
{
	switch ( event.type )
	{
	case SDL_JOYHATMOTION: // DPAD
	{
		SDL_JoystickID controllerID = event.cbutton.which;
		Uint8 hat = event.jhat.hat;
		Uint8 value = event.jhat.value;

		if ( value == mLastHatState ) break;

		auto& keyMappings = Core::GetInstance( )->GetKeyMappings( );

		for ( Core::KeyInstance* key : keyMappings[Core::EView::Emulator][Core::EInputType::JoyHat] )
		{
			bool lastStateDown = mLastHatState & key->index;
			bool stateDown = value & key->index;
			
			if ( lastStateDown && !stateDown )
			{
				ButtonPress( key->key, false );
			}
			if ( !lastStateDown && stateDown )
			{
				ButtonPress( key->key, true );
			}
		}
		mLastHatState = value;
		std::cout << "DPAD: " << controllerID << " " << (int) hat << " " << (int) value << std::endl;
		break;
	}
	case SDL_JOYAXISMOTION:// Joystick
	{
		SDL_JoystickID controllerID = event.cbutton.which;
		Uint8 axis = event.caxis.axis;
		Sint16 value = event.caxis.value;

		if ( mAxisLastRange.find( axis ) == mAxisLastRange.end( ) )
		{
			mAxisLastRange[axis] = 0;
		}

		auto& keyMappings = Core::GetInstance( )->GetKeyMappings( );

		for ( Core::KeyInstance* key : keyMappings[Core::EView::Emulator][Core::EInputType::JoyAxis] )
		{

			if ( key->index == axis )
			{
				bool lastPressed = key->startRange > 0 ? mAxisLastRange[axis] > key->startRange : mAxisLastRange[axis] < key->startRange;
				bool pressed = key->startRange > 0 ? value > key->startRange : value < key->startRange;

				if ( lastPressed && !pressed )
				{
					ButtonPress( key->key, false );
				}
				if ( !lastPressed && pressed )
				{
					ButtonPress( key->key, true );
				}
			}
		}
		mAxisLastRange[axis] = value;

		//std::cout << "Axis: " << controllerID << " " << (int) axis << " " << (int) value << std::endl;
		break;
	}
	case SDL_JOYBUTTONDOWN: // Joypad Button Down
	case SDL_JOYBUTTONUP: // Joypad Button Up
	{
		SDL_JoystickID controllerID = event.cbutton.which;
		Uint8 button = event.cbutton.button;
		bool keyDown = event.type == SDL_JOYBUTTONDOWN;

		auto& keyMappings = Core::GetInstance( )->GetKeyMappings( );

		for ( Core::KeyInstance* key : keyMappings[Core::EView::Emulator][Core::EInputType::JoyButton] )
		{
			if ( key->index == button )
			{
				ButtonPress( key->key, keyDown );
			}
		}
		std::cout << "Button Up: " << controllerID << " " << (int) button << std::endl;
		break;
	}

	case SDL_KEYDOWN: // Keyboard
	case SDL_KEYUP: // Keyboard
	{
		bool keyDown = event.type == SDL_KEYDOWN;
		int keyCode = event.key.keysym.scancode;

		auto& keyMappings = Core::GetInstance( )->GetKeyMappings( );

		for ( Core::KeyInstance* key : keyMappings[Core::EView::Emulator][Core::EInputType::Keyboard] )
		{
			if ( key->index == keyCode )
			{
				ButtonPress( key->key, keyDown );
			}
		}
		break;
	}
	}
}

EmulationManager* EmulationManager::GetInstance( )
{
	return mInstance;
}
