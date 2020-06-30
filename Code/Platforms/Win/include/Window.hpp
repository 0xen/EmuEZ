#pragma once

#include <SDL.h>
#include <SDL_syswm.h>
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

private:

	// Window information
	SDL_Window* window;
	SDL_SysWMinfo window_info;
	uint32_t window_width;
	uint32_t window_height;

	// Clear Color
	float clear_color[4];

	EWindowStatus windowStatus = EWindowStatus::Ready;
};