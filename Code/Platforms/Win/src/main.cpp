#include <thread>
#include <mutex>
#include <iostream>
#include <SDL.h>
#include <SDL_syswm.h>
#include <assert.h>

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

enum class EWindowStatus
{
	Closed,
	Ready,
	Open,
	Closing
};

WorkerMutex workerMutex;
bool requestEmulatorStop = false;

std::thread* emulatorThread = nullptr;
ESystemStatus enulatorStatus;

// Window information
static SDL_Window* window;
static SDL_Renderer* renderer;
static SDL_Texture* screen_texture;
EWindowStatus windowStatus = EWindowStatus::Closed;
int screen_width = 0;
int screen_height = 0;
char* emuScreenBuffer = nullptr;
unsigned int emuScreenBufferSize = 0;

void WindowSetup(const char* title, int width, int height)
{
	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow(
		title,
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		width, height,
		SDL_WINDOW_ALLOW_HIGHDPI
	);
	// We create a renderer that is forced in sync with the refresh rate
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	screen_texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		width, height
	);

	// Dissabled refresh rate
	//SDL_SetWindowDisplayMode(window, NULL);

	SDL_ShowWindow(window);
}

void PollWindow()
{
	// Poll Window
	SDL_Event event;
	while (SDL_PollEvent(&event) > 0)
	{
		switch (event.type)
		{
		case SDL_QUIT:
			windowStatus = EWindowStatus::Closing;
			break;
		case SDL_WINDOWEVENT:
			switch (event.window.event)
			{
			case SDL_WINDOWEVENT_SIZE_CHANGED:

				break;
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			break;

		case SDL_MOUSEMOTION:
		{

		}
		break;
		case SDL_TEXTINPUT:
		{
			break;
		}
		case SDL_MOUSEWHEEL:
		{
			break;
		}
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		{
			//int key = event.key.keysym.scancode;
			//KeyInput(key, (event.type == SDL_KEYDOWN));

		}
		break;
		break;
		}
	}
}
void DestroyWindow()
{
	SDL_DestroyWindow(window);
}

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
		windowStatus = EWindowStatus::Ready;
		screen_width = e.ScreenWidth();
		screen_height = e.ScreenHeight();
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
			e.GetScreenBuffer(emuScreenBuffer, emuScreenBufferSize);
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

	if (windowStatus == EWindowStatus::Open)
	{
		PollWindow();

		std::cout << "Output to screen" << std::endl;

		void* pixels_ptr;
		int pitch;
		SDL_LockTexture(screen_texture, nullptr, &pixels_ptr, &pitch);

		char* pixels = static_cast<char*>(pixels_ptr);

		memcpy(pixels, emuScreenBuffer, emuScreenBufferSize);
		SDL_UnlockTexture(screen_texture);
		SDL_RenderCopy(renderer, screen_texture, nullptr, nullptr);
		SDL_RenderPresent(renderer);
	}






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

	if (windowStatus == EWindowStatus::Ready)
	{
		WindowSetup("EmuEZ", screen_width, screen_height);
		windowStatus = EWindowStatus::Open;
	}
	else
	{
		assert(0 && "Window not reporting ready, confused....");
	}

	return true;
}

int main(int, char**)
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

	if (windowStatus == EWindowStatus::Closing || windowStatus == EWindowStatus::Open)
	{
		windowStatus = EWindowStatus::Closed;
		DestroyWindow();
	}

	std::cout << "Ended" << std::endl;

	system("pause");
	return 0;
}