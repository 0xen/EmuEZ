#include <EmulationManager.hpp>

#include <Visualisation.hpp>
#include <Window.hpp>
#include <Core.hpp>

#include <GB.hpp>
#include <PSX.hpp>

EmulationManager::GameboyConfig EmulationManager::mGameboy;
EmulationManager* EmulationManager::mInstance = nullptr;


void GameWindowInputEvent( ConsoleKeys key, bool pressed )
{
	EmulationManager::GetInstance( )->GameInputEvent( key,pressed );
}


EmulationManager::EmulationManager( EGame game, EmuWindow* window ) : mGame( game ), pWindow( window )
{
	mInstance = this;
	mStatus = (int) EEmulatorFlags::Stopped;
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
		while (mStatus == (int) EEmulatorFlags::Stopped)
		{
			mMutex.condition.wait( lock );
		};
	}
}

void EmulationManager::Stop()
{
	{
		std::unique_lock<std::mutex> lock( mMutex.mutex ); 

		while ( mStatus != (int) EEmulatorFlags::Stopped )
		{
			if (mMutex.ready)
			{
				mMutex.ready = false;
				mStatus |= (int) EEmulatorFlags::StopRequested;
				mMutex.condition.notify_one();
			}
			mMutex.condition.wait( lock );
		};

		mThread.join();
	}
}

void EmulationManager::RequestAction( EEmulatorFlags flag )
{
	{
		std::unique_lock<std::mutex> lock( mMutex.mutex );
		mStatus |= (int) flag;
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

void EmulationManager::GameInputEvent( ConsoleKeys key, bool pressed )
{
	ButtonPress( key, pressed );
}

EmulationManager* EmulationManager::GetInstance( )
{
	return mInstance;
}
