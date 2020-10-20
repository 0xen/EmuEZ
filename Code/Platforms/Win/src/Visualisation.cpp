#include <Visualisation.hpp>

#include <Visualisation.hpp>
#include <Renderer.hpp>

#include <UI.hpp>

#include <memory>

#include <SDL.h>
#include <SDL_syswm.h>

Visualisation::Visualisation( unsigned int windowWidth, unsigned int windowHeight ) :
	mWindowWidth( windowWidth ), mWindowHeight( windowHeight )
{

}

Visualisation::~Visualisation()
{

}

void Visualisation::SetPixels( char* data, unsigned int size )
{
	EmuRender::instance->TransferToGPUBuffer( EmuUI::instance->visualisation_texture_buffer, data, size );
	EmuRender::instance->TransferToGPUTexture( EmuUI::instance->visualisation_texture_buffer, EmuUI::instance->visualisation_texture );
}

void Visualisation::ClearScreen()
{
	const unsigned int size = 160 * 144 * 4;
	char data[size];
	
	for (int i = 0; i < size; i++)
	{
		data[i] = 0;
	}

	EmuRender::instance->TransferToGPUBuffer( EmuUI::instance->visualisation_texture_buffer, data, size );
	EmuRender::instance->TransferToGPUTexture( EmuUI::instance->visualisation_texture_buffer, EmuUI::instance->visualisation_texture );
}
