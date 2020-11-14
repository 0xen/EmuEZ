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

enum class EEmulatorFlags
{
	Stopped = 0,
	FileError = 1 << 0,
	IOError = 1 << 1,
	StopRequested = 1 << 2,
	SaveState = 1 << 3,
	LoadState = 1 << 4,
	Running = 1 << 5
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

	void RequestAction( EEmulatorFlags flag );

	static void Save( pugi::xml_node& node );

	static void Load( pugi::xml_node& node );

	static GameboyConfig& GetGameboyConfig();

	void ButtonPress( ConsoleKeys key, bool pressed );

	void GameInputEvent( ConsoleKeys key, bool pressed );

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
	int mStatus;

	char* mEmuScreenBuffer = nullptr;
	unsigned int mEmuScreenBufferSize = 0;

	std::mutex mQueuedKeysMutex;
	std::vector<std::pair<ConsoleKeys, bool>> mQueuedKeys;
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
			mStatus |= (int) EEmulatorFlags::FileError;
			mMutex.condition.notify_one();
			return;
		}
		e.SkipBIOS();

		{ // Load Game Ram
			std::stringstream ss;
			ss << ".\\Saves\\" << mGame.name << ".sav";
			if ( std::filesystem::exists( ss.str( ) ) )
			{
				std::ifstream  infile( ss.str( ), std::ifstream::binary );
				e.Load( SaveType::PowerDown, infile );
				infile.close( );
			}
		}
		

		mStatus |= (int) EEmulatorFlags::Running;
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
				// Save State
				if ( (mStatus & (int) EEmulatorFlags::SaveState) == (int) EEmulatorFlags::SaveState )
				{

					mStatus &= ~((int)EEmulatorFlags::SaveState);
					// Save State
					{
						std::stringstream ss;
						ss << ".\\SaveStates\\" << mGame.name << ".sav";
						std::ofstream outfile( ss.str( ), std::ofstream::binary );
						e.Save( SaveType::SaveState, outfile );
						outfile.close( );
					}
				}

				if ( (mStatus & (int) EEmulatorFlags::LoadState) == (int) EEmulatorFlags::LoadState )
				{
					mStatus &= ~((int) EEmulatorFlags::LoadState);
					// Load Save State
					std::stringstream ss;
					ss << ".\\SaveStates\\" << mGame.name << ".sav";
					if ( std::filesystem::exists( ss.str( ) ) )
					{
						std::ifstream  infile( ss.str( ), std::ifstream::binary );
						e.Load( SaveType::SaveState, infile );
						infile.close( );
					}
				}

				if ((mStatus & (int) EEmulatorFlags::StopRequested) == (int) EEmulatorFlags::StopRequested )
				{
					mStatus = (int) EEmulatorFlags::Stopped;
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
