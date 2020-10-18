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

	ImGuiIO& io = ImGui::GetIO();
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
				io.DisplaySize = ImVec2( event.window.data1, event.window.data2 );

				break;
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			if (event.button.button == SDL_BUTTON_LEFT) io.MouseDown[0] = event.button.state == SDL_PRESSED;
			if (event.button.button == SDL_BUTTON_RIGHT) io.MouseDown[1] = event.button.state == SDL_PRESSED;
			if (event.button.button == SDL_BUTTON_MIDDLE) io.MouseDown[2] = event.button.state == SDL_PRESSED;
			break;

		case SDL_MOUSEMOTION:
		{
			io.MousePos = ImVec2( event.motion.x, event.motion.y );

		}
		break;
		case SDL_TEXTINPUT:
		{
			io.AddInputCharactersUTF8( event.text.text );
			break;
		}
		case SDL_MOUSEWHEEL:
		{
			if (event.wheel.x > 0) io.MouseWheelH += 1;
			if (event.wheel.x < 0) io.MouseWheelH -= 1;
			if (event.wheel.y > 0) io.MouseWheel += 1;
			if (event.wheel.y < 0) io.MouseWheel -= 1;
			break;
		}
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		{
			int key = event.key.keysym.scancode;
			IM_ASSERT( key >= 0 && key < IM_ARRAYSIZE( io.KeysDown ) );
			{
				io.KeysDown[key] = (event.type == SDL_KEYDOWN);
			}
			io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
			io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
			io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
			io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);

		}
		break;
		break;
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
