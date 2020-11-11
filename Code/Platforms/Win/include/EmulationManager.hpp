#pragma once

#include <thread>
#include <mutex>


#include <pugixml.hpp>
#include <SDL.h>
#include <SDL_syswm.h>

#include <Base.hpp>

#include <iostream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <map>

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

enum class EEmulator
{
	GB,
	PSX
};

struct EGame
{
	EEmulator emulator;
	std::string path;
	std::string name;
};

class Visualisation;
class EmuWindow;

class EmulationManager
{
	// Emulator spesific settings
	// Gameboy
	static struct GameboyConfig
	{
		bool mSkipBIOS;
	}mGameboy;
public:
	EmulationManager( EGame game, EmuWindow* window );
	~EmulationManager();

	void SyncEmulator( Visualisation* visualisation );

	unsigned int GetScreenWidth();

	unsigned int GetScreenHeight();

	void WaitTillReady();

	void Stop();

	static void Save( pugi::xml_node& node );

	static void Load( pugi::xml_node& node );

	static GameboyConfig& GetGameboyConfig();

	void ButtonPress( ConsoleKeys key, bool pressed );

	void GameInputEvent( SDL_Event& event );

	static EmulationManager* GetInstance();
private:

	static EmulationManager* mInstance;

	template <typename emu>
	void EmulationLoop();


	EGame mGame;
	EmuWindow* pWindow;
	unsigned int mScreenWidth = 0;
	unsigned int mScreenHeight = 0;
	std::thread mThread;
	WorkerMutex mMutex;
	EEmulatorStatus mStatus;

	char* mEmuScreenBuffer = nullptr;
	unsigned int mEmuScreenBufferSize = 0;

	std::mutex mQueuedKeysMutex;
	std::vector<std::pair<ConsoleKeys, bool>> mQueuedKeys;

	std::map<int, int> mAxisLastRange;
	int mLastHatState;
};

template<typename emu>
inline void EmulationManager::EmulationLoop()
{
	emu e;

	{
		std::unique_lock<std::mutex> lock( mMutex.mutex );
		if (!e.Init( mGame.path.c_str() ))
		{
			// Could not find game!
			mStatus = EEmulatorStatus::FileError;
			mMutex.condition.notify_one();
			return;
		}
		e.SkipBIOS();

		{ // Load Game
			std::stringstream ss;
			ss << ".\\Saves\\" << mGame.name << ".sav";
			if ( std::filesystem::exists( ss.str( ) ) )
			{
				std::ifstream  infile( ss.str( ), std::ifstream::binary );
				e.Load( SaveType::PowerDown, infile );
				infile.close( );
			}
		}
		

		mStatus = EEmulatorStatus::Running;
		mScreenWidth = e.ScreenWidth();
		mScreenHeight = e.ScreenHeight();
		mMutex.condition.notify_one();
	}

	Uint32 startTime = SDL_GetTicks();
	Uint32 endTime = 0;
	Uint32 delta = 0;
	Uint32 fps = 30;
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

					// Save RAM / Game traditional save state
					{
						std::stringstream ss;
						ss << ".\\Saves\\" << mGame.name << ".sav";	
						std::ofstream outfile( ss.str(), std::ofstream::binary );
						e.Save( SaveType::PowerDown, outfile );
						outfile.close( );
					}

					return;
				}
				mMutex.condition.wait( lock );
			};
		}

		{

			std::unique_lock<std::mutex> lock( mQueuedKeysMutex );


			for ( std::pair<ConsoleKeys, bool>& pair : mQueuedKeys )
			{
				if ( pair.second )
				{
					e.KeyPress( pair.first );
				}
				else
				{
					e.KeyRelease( pair.first );
				}
			}
			mQueuedKeys.clear( );
		}

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
