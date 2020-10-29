#include <EmulationManager.hpp>

#include <Visualisation.hpp>

#include <GB.hpp>
#include <PSX.hpp>

EmulationManager::EmulationManager( EGame game ) : mGame( game )
{
	mStatus = EEmulatorStatus::Stopped;
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
