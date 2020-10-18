#include <Visualisation.hpp>

#include <Visualisation.hpp>
#include <Renderer.hpp>

#include <UI.hpp>

#include <memory>

#include <SDL.h>
#include <SDL_syswm.h>

Visualisation::Visualisation( EVisualisationMode mode, unsigned int windowWidth, unsigned int windowHeight, unsigned int viewportWidth, unsigned int viewportHeight ) :
	mMode( mode ), mWindowWidth( windowWidth ), mWindowHeight( windowHeight ), mViewportWidth( viewportWidth ), mViewportHeight( viewportHeight )
{
	SetupWindow();
}

Visualisation::~Visualisation()
{
	DestroyWindow();
}

void Visualisation::SetupWindow()
{

	switch (mMode)
	{
		case EVisualisationMode::Windowed:
		{
			window = SDL_CreateWindow(
				"Game",
				SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
				mWindowWidth, mWindowHeight,
				SDL_WINDOW_ALLOW_HIGHDPI
			);

			// We create a renderer that is forced in sync with the refresh rate
			renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED );

			screen_texture = SDL_CreateTexture(
				renderer,
				SDL_PIXELFORMAT_ARGB8888,
				SDL_TEXTUREACCESS_STREAMING,
				mViewportWidth, mViewportHeight
			);

			SDL_ShowWindow( window );
			break;
		}
		case EVisualisationMode::ImGUI:
		{
			break;
		}
	}


	
}

void Visualisation::DestroyWindow()
{
	switch (mMode)
	{
		case EVisualisationMode::Windowed:
		{
			SDL_DestroyWindow( window );
			break;
		}
		case EVisualisationMode::ImGUI:
		{

			break;
		}
	}
}

void Visualisation::Update()
{
	switch (mMode)
	{
		case EVisualisationMode::Windowed:
		{
			SDL_Event event;

			while (SDL_PollEvent( &event ) > 0)
			{
				switch (event.type)
				{
				case SDL_QUIT:

					break;
				case SDL_WINDOWEVENT:
					switch (event.window.event)
					{
					case SDL_WINDOWEVENT_SIZE_CHANGED:

						break;
					}
				}
			}
			break;
		}
		case EVisualisationMode::ImGUI:
		{
			break;
		}
	}

}

void Visualisation::SetPixels( char* data, unsigned int size )
{
	switch (mMode)
	{
		case EVisualisationMode::Windowed:
		{
			void* pixels_ptr;
			int pitch;
			SDL_LockTexture( screen_texture, nullptr, &pixels_ptr, &pitch );

			char* pixels = static_cast<char*>(pixels_ptr);

			memcpy( pixels, data, size );
			SDL_UnlockTexture( screen_texture );
			SDL_RenderCopy( renderer, screen_texture, nullptr, nullptr );
			SDL_RenderPresent( renderer );
			break;
		}
		case EVisualisationMode::ImGUI:
		{
			EmuRender::instance->TransferToGPUBuffer( EmuUI::instance->visualisation_texture_buffer, data, size );
			EmuRender::instance->TransferToGPUTexture( EmuUI::instance->visualisation_texture_buffer, EmuUI::instance->visualisation_texture );
			break;
		}
	}

}

void Visualisation::ClearScreen()
{
	const unsigned int size = 160 * 144 * 4;
	char data[size];
	
	for (int i = 0; i < size; i++)
	{
		data[i] = 0;
	}

	switch (mMode)
	{
		case EVisualisationMode::Windowed:
		{
			void* pixels_ptr;
			int pitch;
			SDL_LockTexture( screen_texture, nullptr, &pixels_ptr, &pitch );

			char* pixels = static_cast<char*>(pixels_ptr);

			memcpy( pixels, data, size );
			SDL_UnlockTexture( screen_texture );
			SDL_RenderCopy( renderer, screen_texture, nullptr, nullptr );
			SDL_RenderPresent( renderer );
			break;
		}
		case EVisualisationMode::ImGUI:
		{
			EmuRender::instance->TransferToGPUBuffer( EmuUI::instance->visualisation_texture_buffer, data, size );
			EmuRender::instance->TransferToGPUTexture( EmuUI::instance->visualisation_texture_buffer, EmuUI::instance->visualisation_texture );
			break;
		}
	}
}

Visualisation::EVisualisationMode Visualisation::GetMode()
{
	return mMode;
}

void Visualisation::SetMode( EVisualisationMode mode )
{
	if (mMode != mode)
	{
		DestroyWindow();

		ClearScreen();

		mMode = mode;

		SetupWindow();
	}
}
