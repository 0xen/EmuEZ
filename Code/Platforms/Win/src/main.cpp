#include <thread>
#include <mutex>
#include <iostream>
#include <GB.hpp>

struct WorkerMutex
{
	std::condition_variable condition;
	std::mutex mutex;
	bool ready;
};

enum class ESystem
{
	GameBoy
};

enum class ESystemStatus
{
	Stopped,
	Running,
	FileError,
	IOError
};

WorkerMutex workerMutex;
bool requestEmulatorStop = false;

std::thread* emulatorThread = nullptr;
ESystemStatus enulatorStatus;

template <typename emu>
void Emulator(const char* path)
{
	workerMutex.ready = false;
	emu e;
	{
		std::unique_lock<std::mutex> lock(workerMutex.mutex);
		if (!e.Init(path))
		{
			// Could not find game!
			enulatorStatus = ESystemStatus::FileError;
			workerMutex.condition.notify_one();
			return;
		}
		enulatorStatus = ESystemStatus::Running;
		workerMutex.condition.notify_one();
	}
	

	while(true)
	{
		{
			std::unique_lock<std::mutex> lock(workerMutex.mutex);
			while (workerMutex.ready)
			{
				if (requestEmulatorStop) 
					return;
				workerMutex.condition.wait(lock);
			};
		}

		e.Tick();

		{
			std::unique_lock<std::mutex> lock(workerMutex.mutex);
			workerMutex.ready = true;
		}
		workerMutex.condition.notify_one();
	}
}

void StopEmulator()
{
	{ // Scope required to stop join/emulator deadlocking
		std::unique_lock<std::mutex> lock(workerMutex.mutex);
		requestEmulatorStop = true;
	}
	if(emulatorThread)
		emulatorThread->join();
}

void RenderFrame()
{
	{
		std::unique_lock<std::mutex> lock(workerMutex.mutex);
		while (!workerMutex.ready)
		{
			workerMutex.condition.wait(lock);
		};
	}

	std::cout << "Output to screen" << std::endl;

	{
		std::unique_lock<std::mutex> lock(workerMutex.mutex);
		workerMutex.ready = false;
	}
	workerMutex.condition.notify_one();
}


bool LoadSystem(ESystem system, const char* path)
{
	if (emulatorThread) delete emulatorThread;

	switch (system)
	{
	case ESystem::GameBoy:
		{
			emulatorThread = new std::thread(Emulator<EmuGB>, path);
			break;
		}
		default:
		{
			// Error check
			break;
		}
	}
	// Wait for system to load
	ESystemStatus loadResponce;
	{
		{
			std::unique_lock<std::mutex> lock(workerMutex.mutex);
			while ((loadResponce = enulatorStatus) == ESystemStatus::Stopped)
			{
				workerMutex.condition.wait(lock);
			};
		}
	}

	switch (loadResponce)
	{
	case ESystemStatus::FileError:
	{
		StopEmulator();
		return false;
		break;
	}
	default:
		// Unhandled Error
		break;
	}

	return true;
}

int main()
{
	enulatorStatus = ESystemStatus::Stopped;

	bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/Tetris.gb");

	// If the system could not load
	if (!systemLoaded)
	{
		printf("Unable to load System\n");
		system("pause");
		return 0;
	}

	
	//for (int i = 0; i < 100; i++)
	while(true)
	{
		RenderFrame();
	}

	StopEmulator();


	std::cout << "Ended" << std::endl;

	system("pause");
	return 0;
}