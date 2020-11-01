#pragma once

#include <SDL.h>
#include <SDL_syswm.h>

#include <vector>
#include <functional>

class EmuWindow
{
public:

	enum class EWindowStatus
	{
		Ready,
		Open,
		Closed,
		Exiting
	};
	enum EInputEventSubsystem
	{
		UI,
		Game
	};

	EmuWindow(const char* title, unsigned int width, unsigned int height);

	~EmuWindow();

	void OpenWindow();

	void CloseWindow();

	void Poll();

	EWindowStatus GetStatus();

	uint32_t GetWidth();

	uint32_t GetHeight();

	SDL_Window* GetWindow();

	SDL_SysWMinfo GetWindowInfo();

	float* GetClearColor();

	void RegisterWindowPoll( std::function<void( SDL_Event& )> func );

	void RegisterInputEventCallback( EInputEventSubsystem subSystem,  std::function<void( SDL_Event& )> func );

	void UnregisterInputEventCallback( EInputEventSubsystem subSystem );
private:

	void RegisterInputEvent( SDL_Event event );

	//friend class Visualisation;

	// Window information
	SDL_Window* window;
	SDL_SysWMinfo window_info;
	uint32_t window_width;
	uint32_t window_height;

	// Controller Handlers
	std::unordered_map<unsigned int, SDL_Joystick* > mControllers;

	// Clear Color
	float clear_color[4];

	std::unordered_map<EInputEventSubsystem, std::function<void( SDL_Event& )>> mInputEventCallbacks;

	std::vector<std::function<void( SDL_Event& )>> mPollCallbacks;

	EWindowStatus windowStatus = EWindowStatus::Ready;
};