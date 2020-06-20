#include <thread>
#include <mutex>
#include <iostream>
#include <SDL.h>
#include <SDL_syswm.h>
#include <map>
#include <assert.h>

#include <GB.hpp>

// Output when the logic steps happen on threads
#define VERBOSE_LOGIC 1

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
bool requestApplicationClose = false;
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
		160, 144
	);

	// Dissabled refresh rate
	//SDL_SetWindowDisplayMode(window, NULL);

	SDL_ShowWindow(window);
}

std::map<int, EmuGB::ConsoleKeys> keys = {
	{ SDL_SCANCODE_W , EmuGB::ConsoleKeys::UP },
	{ SDL_SCANCODE_S , EmuGB::ConsoleKeys::DOWN },
	{ SDL_SCANCODE_A , EmuGB::ConsoleKeys::LEFT },
	{ SDL_SCANCODE_D , EmuGB::ConsoleKeys::RIGHT },

	{ SDL_SCANCODE_X , EmuGB::ConsoleKeys::A },
	{ SDL_SCANCODE_Z , EmuGB::ConsoleKeys::B },
	{ SDL_SCANCODE_Q , EmuGB::ConsoleKeys::SELECT },
	{ SDL_SCANCODE_E , EmuGB::ConsoleKeys::START },
};

struct KeyRecording
{
	int key;
	bool state;
};
std::mutex frameKeysMutex;
const unsigned int kMaxFrameKeys = 10;
KeyRecording frameKeys[kMaxFrameKeys];
unsigned int frameKeysCount = 0;

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
			requestApplicationClose = true;
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
			std::unique_lock<std::mutex> lock(frameKeysMutex);
			if (frameKeysCount >= kMaxFrameKeys)
				break;
			int key = event.key.keysym.scancode;
			frameKeys[frameKeysCount++] = { key, (event.type == SDL_KEYDOWN) };
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

#if VERBOSE_LOGIC
		std::cout << "Ticking Emulator Input" << std::endl;
#endif
		{
			std::unique_lock<std::mutex> lock(frameKeysMutex);

			for (unsigned int i = 0; i < frameKeysCount; ++i)
			{
				KeyRecording& key = frameKeys[i];
				if (keys.find(key.key) == keys.end() || 
					(key.state && e.IsKeyDown(keys[key.key])) || 
					(!key.state && !e.IsKeyDown(keys[key.key]))) continue;

				if (key.state)
				{
					e.KeyPress(keys[key.key]);
				}
				else
				{
					e.KeyRelease(keys[key.key]);
				}
				
			}
			frameKeysCount = 0;
		}



#if VERBOSE_LOGIC
		std::cout << "Ticking Emulator" << std::endl;
#endif
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

		if (windowStatus != EWindowStatus::Closing)
		{
#if VERBOSE_LOGIC
			std::cout << "Output to screen" << std::endl;
#endif
			void* pixels_ptr;
			int pitch;
			SDL_LockTexture(screen_texture, nullptr, &pixels_ptr, &pitch);

			char* pixels = static_cast<char*>(pixels_ptr);

			memcpy(pixels, emuScreenBuffer, emuScreenBufferSize);
			SDL_UnlockTexture(screen_texture);
			SDL_RenderCopy(renderer, screen_texture, nullptr, nullptr);
			SDL_RenderPresent(renderer);
		}
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
		WindowSetup("EmuEZ", screen_width*5, screen_height*5);
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

	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/Tetris.gb");
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/DrMario.gb");
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/loz.gb");
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/PokemonRed.gb");



	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/cpu_instrs.gb");
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/individual/01-special.gb");
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/individual/02-interrupts.gb"); //#
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/individual/03-op sp,hl.gb"); //#
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/individual/04-op r,imm.gb");
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/individual/05-op rp.gb");
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/individual/06-ld r,r.gb");
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/individual/07-jr,jp,call,ret,rst.gb");
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/individual/08-misc instrs.gb");
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/individual/09-op r,r.gb");
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/individual/10-bit ops.gb");
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/individual/11-op a,(hl).gb");

	

	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/mooneye/emulator-only/mbc1/bits_bank1.gb");
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/mooneye/emulator-only/mbc1/bits_bank2.gb"); //#
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/mooneye/emulator-only/mbc1/bits_mode.gb");
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/mooneye/emulator-only/mbc1/bits_ramg.gb");
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/mooneye/emulator-only/mbc1/multicart_rom_8Mb.gb"); //#
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/mooneye/emulator-only/mbc1/ram_64kb.gb");
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/mooneye/emulator-only/mbc1/ram_256kb.gb"); //#
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/mooneye/emulator-only/mbc1/rom_1Mb.gb");
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/mooneye/emulator-only/mbc1/rom_2Mb.gb");
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/mooneye/emulator-only/mbc1/rom_4Mb.gb");
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/mooneye/emulator-only/mbc1/rom_8Mb.gb");//#
	//bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/mooneye/emulator-only/mbc1/rom_16Mb.gb");//#
	bool systemLoaded = LoadSystem(ESystem::GameBoy, "Games/GB/mooneye/emulator-only/mbc1/rom_512kb.gb");
	


	

	// If the system could not load
	if (!systemLoaded)
	{
		printf("Unable to load System\n");
		system("pause");
		return 0;
	}

	
	while(!requestApplicationClose)
	{
		RenderFrame();
	}

	StopEmulator();

	if (windowStatus == EWindowStatus::Closing || windowStatus == EWindowStatus::Open)
	{
		windowStatus = EWindowStatus::Closed;
		DestroyWindow();
	}

#if VERBOSE_LOGIC
	std::cout << "Applciation Ended" << std::endl;
#endif

	system("pause");
	return 0;
}