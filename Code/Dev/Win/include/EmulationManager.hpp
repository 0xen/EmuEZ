#pragma once

#include <thread>
#include <mutex>

#include <SDL.h>
#include <SDL_syswm.h>

#include <iostream>

struct WorkerMutex
{
	std::condition_variable condition;
	std::mutex mutex;
	bool ready;
};

enum class EEmulatorStatus
{
	Stopped,
	Running,
	FileError,
	IOError,
	StopRequested
};

class Visualisation;

class EmulationManager
{
public:
	EmulationManager(const char* path);
	~EmulationManager();

	void SyncEmulator( Visualisation* visualisation );

	unsigned int GetScreenWidth();

	unsigned int GetScreenHeight();

	void WaitTillReady();

	void Stop();
private:

	template <typename emu>
	void EmulationLoop();


	const char* mPath;
	unsigned int mScreenWidth = 0;
	unsigned int mScreenHeight = 0;
	std::thread mThread;
	WorkerMutex mMutex;
	EEmulatorStatus mStatus;

	char* mEmuScreenBuffer = nullptr;
	unsigned int mEmuScreenBufferSize = 0;
};

template<typename emu>
inline void EmulationManager::EmulationLoop()
{
	emu e;

	{
		std::unique_lock<std::mutex> lock( mMutex.mutex );
		if (!e.Init( mPath ))
		{
			// Could not find game!
			mStatus = EEmulatorStatus::FileError;
			mMutex.condition.notify_one();
			return;
		}
		mStatus = EEmulatorStatus::Running;
		mScreenWidth = e.ScreenWidth();
		mScreenHeight = e.ScreenHeight();
		mMutex.condition.notify_one();
	}

	Uint32 startTime = SDL_GetTicks();
	Uint32 endTime = 0;
	Uint32 delta = 0;
	Uint32 fps = 60;
	Uint32 timePerFrame = 1000 / fps;


	while (true)
	{
		{
			std::unique_lock<std::mutex> lock( mMutex.mutex );
			while (mMutex.ready)
			{
				if (mStatus == EEmulatorStatus::StopRequested)
				{
					mStatus = EEmulatorStatus::Stopped;
					lock.unlock();
					mMutex.condition.notify_one();
					return;
				}
				mMutex.condition.wait( lock );
			};
			std::cout << "Emu" << std::endl;
		}

		/*{
			std::unique_lock<std::mutex> lock( frameKeysMutex );

			for (unsigned int i = 0; i < frameKeysCount; ++i)
			{
				KeyRecording& key = frameKeys[i];
				if (keys.find( key.key ) == keys.end() ||
					(key.state && e.IsKeyDown( keys[key.key] )) ||
					(!key.state && !e.IsKeyDown( keys[key.key] ))) continue;

				if (key.state)
				{
					e.KeyPress( keys[key.key] );
				}
				else
				{
					e.KeyRelease( keys[key.key] );
				}

			}
			frameKeysCount = 0;
		}*/


		e.Tick();

		endTime = SDL_GetTicks();

		delta = endTime - startTime;

		{
			std::unique_lock<std::mutex> lock( mMutex.mutex );
			mMutex.ready = true;
			e.GetScreenBuffer( mEmuScreenBuffer, mEmuScreenBufferSize );
		}
		mMutex.condition.notify_one();

		if (delta < timePerFrame)
		{
			SDL_Delay( timePerFrame - delta );
		}

		startTime = endTime;
	}
}
