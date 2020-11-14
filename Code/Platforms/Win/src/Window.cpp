#include <Window.hpp>
#include <Core.hpp>

#include <assert.h>

#include <iostream>

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

	SDL_SetWindowMinimumSize( window, 800, 600 );

	SDL_VERSION(&window_info.version);
	bool sucsess = SDL_GetWindowWMInfo(window, &window_info);
	assert(sucsess && "Error, unable to get window info");
	windowStatus = EWindowStatus::Closed;


	//Initialize SDL
	if (SDL_Init( SDL_INIT_JOYSTICK ) < 0)
	{
		printf( "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
	}

	////Check for joysticks
	//if (SDL_NumJoysticks() < 1)
	//{
	//	printf( "Warning: No joysticks connected!\n" );
	//}
	//else
	//{
	//	//Load joystick
	//	gGameController = SDL_JoystickOpen( 0 );
	//	if (gGameController == NULL)
	//	{
	//		printf( "Warning: Unable to open game controller! SDL Error: %s\n", SDL_GetError() );
	//	}
	//}
}

EmuWindow::~EmuWindow()
{
	for (auto it = mControllers.begin(); it != mControllers.end(); ++it)
	{
		SDL_JoystickClose( it->second );
	}
	mControllers.clear();
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
			case SDL_JOYDEVICEADDED:
			{
				if (mControllers.find( event.cdevice.which ) != mControllers.end())
				{
					std::cout << "Unable to add same controller with id " << event.cdevice.which << std::endl;
					break;
				}
				std::cout << "added" << " " << event.cdevice.which << std::endl;
				mControllers[event.cdevice.which] = SDL_JoystickOpen( event.cdevice.which );
				break;
			}
			case SDL_JOYDEVICEREMOVED:
			{
				if (mControllers.find( event.cdevice.which ) == mControllers.end())
				{
					std::cout << "Unable to remove controller not set up " << event.cdevice.which << std::endl;
					break;
				}
				std::cout << "removed" << " " << event.cdevice.which << std::endl;
				SDL_JoystickClose( mControllers[event.cdevice.which] );
				mControllers.erase( mControllers.find( event.cdevice.which ) );
				break;
			}



			case SDL_JOYHATMOTION: // DPAD
			{
				SDL_JoystickID controllerID = event.cbutton.which;
				Uint8 hat = event.jhat.hat;
				Uint8 value = event.jhat.value;

				if ( value == mLastHatState ) break;

				auto& keyMappings = Core::GetInstance( )->GetKeyMappings( );

				for ( Core::KeyInstance* key : keyMappings[Core::EView::Emulator][Core::EInputType::JoyHat] )
				{
					bool lastStateDown = mLastHatState & key->index;
					bool stateDown = value & key->index;

					if ( lastStateDown && !stateDown )
					{
						RegisterInputEvent( key->key, false );
					}
					if ( !lastStateDown && stateDown )
					{
						RegisterInputEvent( key->key, true );
					}
				}
				mLastHatState = value;
				break;
			}
			case SDL_JOYAXISMOTION:// Joystick
			{
				SDL_JoystickID controllerID = event.cbutton.which;
				Uint8 axis = event.caxis.axis;
				Sint16 value = event.caxis.value;

				if ( mAxisLastRange.find( axis ) == mAxisLastRange.end( ) )
				{
					mAxisLastRange[axis] = 0;
				}

				auto& keyMappings = Core::GetInstance( )->GetKeyMappings( );

				for ( Core::KeyInstance* key : keyMappings[Core::EView::Emulator][Core::EInputType::JoyAxis] )
				{

					if ( key->index == axis )
					{
						bool lastPressed = key->startRange > 0 ? mAxisLastRange[axis] > key->startRange : mAxisLastRange[axis] < key->startRange;
						bool pressed = key->startRange > 0 ? value > key->startRange : value < key->startRange;

						if ( lastPressed && !pressed )
						{
							RegisterInputEvent( key->key, false );
						}
						if ( !lastPressed && pressed )
						{
							RegisterInputEvent( key->key, true );
						}
					}
				}
				mAxisLastRange[axis] = value;
				break;
			}
			case SDL_JOYBUTTONDOWN: // Joypad Button Down
			case SDL_JOYBUTTONUP: // Joypad Button Up
			{
				SDL_JoystickID controllerID = event.cbutton.which;
				Uint8 button = event.cbutton.button;
				bool keyDown = event.type == SDL_JOYBUTTONDOWN;

				auto& keyMappings = Core::GetInstance( )->GetKeyMappings( );

				for ( Core::KeyInstance* key : keyMappings[Core::EView::Emulator][Core::EInputType::JoyButton] )
				{
					if ( key->index == button )
					{
						RegisterInputEvent( key->key, keyDown );
					}
				}
				break;
			}

			case SDL_KEYDOWN: // Keyboard
			case SDL_KEYUP: // Keyboard
			{
				bool keyDown = event.type == SDL_KEYDOWN;
				int keyCode = event.key.keysym.scancode;

				auto& keyMappings = Core::GetInstance( )->GetKeyMappings( );

				for ( Core::KeyInstance* key : keyMappings[Core::EView::Emulator][Core::EInputType::Keyboard] )
				{
					if ( key->index == keyCode )
					{
						RegisterInputEvent( key->key, keyDown );
					}
				}
				break;
			}
			
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

void EmuWindow::RegisterInputEventCallback( EInputEventSubsystem subSystem, std::function<void( ConsoleKeys key, bool pressed )> func )
{
	mInputEventCallbacks[subSystem] = func;
}

void EmuWindow::UnregisterInputEventCallback( EInputEventSubsystem subSystem )
{
	auto it = mInputEventCallbacks.find( subSystem );
	if (it != mInputEventCallbacks.end())
	{
		mInputEventCallbacks.erase( it );
	}
}

void EmuWindow::RegisterInputEvent( ConsoleKeys key, bool pressed )
{
	for (auto it = mInputEventCallbacks.begin(); it != mInputEventCallbacks.end(); ++it)
	{
		it->second( key, pressed );
	}
}

