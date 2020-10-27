#include <Window.hpp>

#include <assert.h>
#include <imgui.h>

EmuWindow::EmuWindow(const char* title, unsigned int width, unsigned int height)
{
	window = SDL_CreateWindow(
		title,
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		width, height,
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN
	);

	window_width = width;
	window_height = height;

	clear_color[2] = clear_color[1] = clear_color[0] = 0.2f;
	clear_color[3] = 1.0f;

	SDL_VERSION(&window_info.version);
	bool sucsess = SDL_GetWindowWMInfo(window, &window_info);
	assert(sucsess && "Error, unable to get window info");
	windowStatus = EWindowStatus::Closed;
}

EmuWindow::~EmuWindow()
{
	SDL_DestroyWindow(window);
}

void EmuWindow::OpenWindow()
{
	windowStatus = EWindowStatus::Open;
	SDL_ShowWindow(window);
}

void EmuWindow::CloseWindow()
{
	windowStatus = EWindowStatus::Exiting;
	SDL_HideWindow(window);
}

void EmuWindow::Poll()
{

	// Poll Window
	SDL_Event event;

	while (SDL_PollEvent( &event ) > 0)
	{
		switch (event.type)
		{
			case SDL_QUIT:
				windowStatus = EWindowStatus::Exiting;
				break;
			case SDL_WINDOWEVENT:
				switch (event.window.event)
				{
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					window_width = event.window.data1;
					window_height = event.window.data2;
					break;
				}
				break;
		}

		for (int i = 0; i < mPollCallbacks.size(); i++)
		{
			mPollCallbacks[i]( event );
		}
	}
	
}

EmuWindow::EWindowStatus EmuWindow::GetStatus()
{
	return windowStatus;
}

uint32_t EmuWindow::GetWidth()
{
	return window_width;
}

uint32_t EmuWindow::GetHeight()
{
	return window_height;
}

SDL_Window* EmuWindow::GetWindow()
{
	return window;
}

SDL_SysWMinfo EmuWindow::GetWindowInfo()
{
	return window_info;
}

float* EmuWindow::GetClearColor()
{
	return clear_color;
}

void EmuWindow::RegisterWindowPoll( std::function<void( SDL_Event& )> func )
{
	mPollCallbacks.push_back( func );
}
