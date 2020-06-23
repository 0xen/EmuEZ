#include <GB.hpp>
#include <IO.hpp>

#include <iostream>
#include <assert.h>

#define DEBUG_TILES 0

EmuGB::EmuGB()
{
	InitOPJumpTables();
}

bool EmuGB::InitEmu(const char* path)
{
	bool loaded = m_cartridge.Load(path, m_bus_memory);
	if (!loaded)return false;

	Reset();

	return true;
}

void EmuGB::TickEmu()
{

	m_cycles = 0;

	bool vSync = false;
	while (!vSync)
	{
		m_cycle = 0;
		if (m_halt)
		{
			m_cycle += (ui16)GetCycleModifier(4);

			if (m_haltDissableCycles > 0)
			{
				m_haltDissableCycles -= m_cycle;

				if (m_haltDissableCycles <= 0)
				{
					m_haltDissableCycles = 0;
					m_halt = false;
				}

			}
			else
			{
				ui8& interupt_flags = ProcessBusReadRef<ui16, ui8>(mk_cpu_interupt_flag_address);
				ui8& interupt_enabled_flags = ProcessBusReadRef<ui16, ui8>(mk_interrupt_enabled_flag_address);

				ui8 interuptsToProcess = interupt_flags & interupt_enabled_flags;

				if (m_halt && interuptsToProcess > 0)
				{
					m_haltDissableCycles = 16;
				}
			}
		}

		if (!m_halt)
		{

			InteruptCheck(); // Interupt checks
			m_op_code = ReadNextOPCode();

			if (m_op_code == 0xCB)
			{
				m_op_code = ReadNextOPCode();

				(this->*m_CBOpCodes[m_op_code])();
			}
			else
			{
				(this->*m_opCodes[m_op_code])();
			}
		}


		TickClock(); // Timers
		vSync = TickDisplay(); // Video
		// ***Audio
		JoypadTick(); // Input

		if (m_IECycles > 0)
		{
			m_IECycles -= m_cycle;

			if (m_IECycles <= 0)
			{
				m_IECycles = 0;
				m_interrupts_enabled = true;
			}
		}

		m_cycles += (ui16)GetCycleModifier(m_cycle);
	}


	m_cartridge.Update();

}

void EmuGB::TickClock()
{
	m_devider_counter += m_cycle;


	unsigned int divider_cycles = 256;

	// Divider
	while (m_devider_counter >= divider_cycles)
	{
		m_devider_counter -= divider_cycles;

		// Dont want to do a normal wright here as it will reset the devider
		ProcessBusReadRef<ui16, ui8>(mk_timer_divider_address)++;
	}
	ui8& timer_control = ProcessBusReadRef<ui16, ui8>(mk_timer_controll_address);


	if (timer_control & 0x04) // Is the timer enabled
	{
		m_timer_counter += m_cycle;


		InitClockFrequency();

		while (m_timer_counter >= m_timer_frequancy)
		{
			m_timer_counter -= m_timer_frequancy;

			ui8& timer = ProcessBusReadRef<ui16, ui8>(mk_timer_address);
			if (timer == 0xFF)
			{
				ui8& timer_modulo = ProcessBusReadRef<ui16, ui8>(mk_timer_modulo_address);
				timer = timer_modulo;
				// Interrupt
				RequestInterupt(CPUInterupt::TIMER);
			}
			else
			{
				timer++;
			}
		}
	}
}

void EmuGB::JoypadTick()
{
	m_joypadCycles += m_cycle;
	if (m_joypadCycles >= joypadCyclesRefresh)
	{
		UpdateJoypad();
		m_joypadCycles = 0;
	}
}

void EmuGB::UpdateJoypad()
{
	ui8 current = ProcessBusReadRef<ui16, ui8>(0xFF00) & 0xF0;

	switch (current & 0x30)
	{
	case 0x10:
	{
		ui8 topJoypad = (joypadActual >> 4) & 0x0F;
		current |= topJoypad;
		break;
	}
	case 0x20:
	{
		ui8 bottomJoypad = joypadActual & 0x0F;
		current |= bottomJoypad;
		break;
	}
	case 0x30:
		current |= 0x0F;
		break;
	}

	if ((ProcessBusReadRef<ui16, ui8>(0xFF00) & ~current & 0x0F) != 0)
		RequestInterupt(CPUInterupt::JOYPAD);

	ProcessBusReadRef<ui16, ui8>(0xFF00) = current;
}

void EmuGB::InitClockFrequency()
{
	switch (ProcessBusReadRef<ui16, ui8>(mk_timer_controll_address) & 0x03)
	{
	case 0:
		m_timer_frequancy = 1024; // Frequency 1024
		break;
	case 1:
		m_timer_frequancy = 16; // Frequency 16
		break;
	case 2:
		m_timer_frequancy = 64; // Frequency 64
		break;
	case 3:
		m_timer_frequancy = 256; // Frequency 256
		break;
	}
}

void EmuGB::ResetDIVCycles()
{
	m_devider_counter = 0;
	ProcessBusReadRef<ui16, ui8>(mk_timer_divider_address) = 0x00;
}

void EmuGB::SetTimerControl(ui8 data)
{
	ui8 current = ProcessBusReadRef<ui16, ui8>(mk_timer_controll_address);
	ProcessBusReadRef<ui16, ui8>(mk_timer_controll_address) = data;
	if ((data & 0x03) != (current & 0x03))
	{
		ProcessBusReadRef<ui16, ui8>(mk_timer_address) = ProcessBusReadRef<ui16, ui8>(mk_timer_modulo_address);
		m_timer_counter = 0;
	}
}

bool EmuGB::TickDisplay()
{


	// Display Status
	ui8& controll_bit = ProcessBusReadRef<ui16, ui8>(mk_controll_byte);
	bool isDisplayEnabled = HasBit(controll_bit, 7);

	ui8& status = ProcessBusReadRef<ui16, ui8>(mk_video_status);
	ui8& line = ProcessBusReadRef<ui16, ui8>(mk_video_line_byte);

	bool vBlank = false;

	m_scanline_counter += m_cycle;

	if (isDisplayEnabled && m_lcd_enabled)
	{
		// Src http://gbdev.gg8.se/wiki/articles/Video_Display#INT_40_-_V-Blank_Interrupt
		// Mode 2  2_____2_____2_____2_____2_____2___________________2____ Scanning OAM
		// Mode 3  _33____33____33____33____33____33__________________3___ Reading OAM
		// Mode 0  ___000___000___000___000___000___000________________000 Horizontal Blank
		// Mode 1  ____________________________________11111111111111_____ Vertical Blank

		// Mode 0 :  Cycles
		// Mode 1 : 4560 Cycles
		// Mode 2 : 80 Cycles
		// Mode 3 :  Cycles

		// Whole frame is 154 scan lines
		switch (m_display_mode)
		{
		case 0: // Horizontal Blank
		{
			if (m_scanline_counter >= 204)
			{
				// Time of mode 0 has elapsed
				m_scanline_counter -= 204;
				// Switch to OAM Scan
				m_display_mode = 2;

				// Increment scan line
				line++;

				// Compare LY to LYC and if they match, interupt
				CompareLYAndLYC();

				if (line == 144) // Are we in VBlank
				{
					// Switch to VBLank
					m_display_mode = 1;

					RequestInterupt(CPUInterupt::VBLANK);

					// Transfer screen data to front buffer
					{
						memcpy(m_front_buffer, m_back_buffer, m_display_buffer_size);

						// Reset back buffer
						for (unsigned int i = 0; i < m_display_buffer_size; i++)
						{
							if (i % 4 == 3)
							{
								m_back_buffer[i] = 255;
							}
							else
							{
								m_back_buffer[i] = 0;
							}
						}
					}

					// Should we V-Blank Interupt
					if (HasBit(status, 4))
					{
						RequestInterupt(CPUInterupt::LCD);
					}
					vBlank = true;
					m_WindowLine = 0;
				}
				else
				{
					// Should we H-Blank Interupt
					if (HasBit(status, 5))
					{
						RequestInterupt(CPUInterupt::LCD);
					}
				}
				UpdateLCDStatus();
			}

			break;
		}
		case 1: // Vertical Blank
		{
			// Have we reached a VBlank line?
			if (m_scanline_counter >= 456)
			{
				m_scanline_counter -= 456;
				line++;
				CompareLYAndLYC();
			}
			// Have we reached the 10th VBlank line?
			if (line > 153)
			{
				line = 0;
				m_display_mode = 2;
				UpdateLCDStatus();
				if (HasBit(status, 5))
				{
					RequestInterupt(CPUInterupt::LCD);
				}
			}
			break;
		}
		case 2: // Scanning OAM
		{
			if (m_scanline_counter >= 80)
			{
				m_scanline_counter -= 80;
				m_scanline_validated = false;
				m_display_mode = 3;
				UpdateLCDStatus();
			}

			break;
		}
		case 3: // Reading OAM
		{


			if (m_oam_pixel < 160)
			{
				m_oam_tile += m_cycle;


				if (HasBit(controll_bit, 7)) // Is the screen on
				{
					// Render the background

					while (m_oam_tile >= 3)
					{

						DrawBG(line, m_oam_pixel, 4);

						m_oam_pixel += 4;
						m_oam_tile -= 3;
						if (m_oam_pixel >= 160)
						{
							break;
						}
					}

				}

			}


			if (m_scanline_counter >= 160 && !m_scanline_validated)
			{


				if (HasBit(controll_bit, 7)) // Is the screen on
				{

					DrawWindow(line);


					// Help from http://www.codeslinger.co.uk/pages/projects/gameboy/graphics.html
					if (HasBit(controll_bit, 1)) // Is Sprites Enabled
					{
						ui8 sprite_height = HasBit(controll_bit, 2) ? 16 : 8;
						//unsigned int line_width = (line * 160); // Gameboy width


						for (i8 sprite = 39; sprite >= 0; --sprite)
						{
							// A sprite takes up 4 bytes in the sprite table
							ui8 index = sprite * 4;
							ui8 yPos = ProcessBusReadRef<ui16, ui8>(0xFE00 + index) - 16;

							if ((yPos > line) || ((yPos + sprite_height) <= line))
								continue;

							int xPos = ProcessBusReadRef<ui16, ui8>(0xFE00 + index + 1) - 8;


							if ((xPos < -7) || (xPos >= 160)) // 160 Gameboy width
								continue;



							ui8 tileLocation = ProcessBusReadRef<ui16, ui8>(0xFE00 + index + 2);
							ui8 attributes = ProcessBusReadRef<ui16, ui8>(0xFE00 + index + 3);



							ui16 colourAddress = HasBit(attributes, 4) ? 0xFF49 : 0xFF48;
							bool xFlip = HasBit(attributes, 5);
							bool yFlip = HasBit(attributes, 6);
							bool aboveBG = !HasBit(attributes, 7);


							//if ((line >= yPos) && (line < (yPos + sprite_height)))
							{

								ui16 spriteLine = 0;
								if (yFlip)
								{
									spriteLine = ((sprite_height == 16) ? 15 : 7) - (line - yPos);
								}
								else
								{
									spriteLine = line - yPos;
								}
								ui16 offset = 0;

								if (sprite_height == 16 && (spriteLine >= 8))
								{
									spriteLine = (spriteLine - 8) * 2;
									offset = 16;
								}
								else
									spriteLine *= 2;


								ui16 dataAddress = (0x8000 + (tileLocation * 16)) + spriteLine + offset;
								ui8 data1 = ProcessBusReadRef<ui16, ui8>(dataAddress);
								ui8 data2 = ProcessBusReadRef<ui16, ui8>(dataAddress + 1);


								for (int tilePixel = 0; tilePixel < 8; ++tilePixel)
								{
									int colorX = xFlip ? tilePixel : 7 - tilePixel;

									// the rest is the same as for tiles
									int colourNum = (ui8)(data2 >> colorX) & 1; // Get the set bit
									colourNum <<= 1;
									colourNum |= (ui8)(data1 >> colorX) & 1; // Get the set bit

									if (colourNum == 0)
										continue;


									int pixel = xPos + tilePixel;




									if (pixel < 0 || (pixel >= 160)) // 160 Gameboy width
										continue;




									int pixel_index = pixel + 160 * line;
									ui8 backgroundColor = m_back_buffer_color_cache[pixel_index];


									// If another sprite has already been drawn at this location, continue
									if (HasBit(backgroundColor, 3))
										continue;

									// If we cant be above a window and the current pixel color for the background is 0, continue.
									// ** Gives a effect of the sprite being behind the window
									if (!aboveBG && (backgroundColor & 0x03))
										continue;

									// Set that a sprite will be drawing in this location to stop other sprites drawing here
									SetBit(backgroundColor, 3);
									m_back_buffer_color_cache[pixel_index] = backgroundColor;

									ui8 palette = ProcessBusReadRef<ui16, ui8>(colourAddress);

									ui8 color = (palette >> (colourNum * 2)) & 0x03;


									pixel_index *= 4;
									m_back_buffer[pixel_index] = m_back_buffer[pixel_index + 1] = m_back_buffer[pixel_index + 2] = (3 - color) * 64;
								}
							}
						}
					}
				}
				else
				{
					for (unsigned int pixel = 0; pixel < ScreenWidthEmu(); ++pixel)
					{
						int pixel_index = pixel + ScreenWidthEmu() * line;
						pixel_index *= 4;
						m_back_buffer[pixel_index] = m_back_buffer[pixel_index + 1] = m_back_buffer[pixel_index + 2] = 0;
					}
				}

				m_scanline_validated = true;
			}


			if (m_scanline_counter >= 172)
			{
				m_scanline_counter -= 172;
				m_display_mode = 0;
				m_oam_pixel = 0;
				m_oam_tile = 0;
				UpdateLCDStatus();
				if (HasBit(status, 3))
				{
					RequestInterupt(CPUInterupt::LCD);
				}
			}






			break;
		}
		}
	}
	else // Screen dissabled
	{
		if (m_display_enable_delay > 0)
		{
			m_display_enable_delay -= m_cycle;

			if (m_display_enable_delay <= 0)
			{
				m_lcd_enabled = true;
				m_display_enable_delay = 0;
				m_scanline_counter = 0;
				ProcessBusReadRef<ui16, ui8>(mk_video_line_byte) = 0;
				m_WindowLine = 0;

				m_display_mode = 0;
				UpdateLCDStatus();

				if (HasBit(status, 5))
				{
					RequestInterupt(CPUInterupt::LCD);
				}
				CompareLYAndLYC();
			}
		}
		else
			// Fake that we have done a vblank when with the screen off
			if (m_scanline_counter >= 70224)
			{
				m_scanline_counter -= 70224;
				vBlank = true;
			}
	}
	return vBlank;
}

void EmuGB::DrawBG(ui8 line, ui8 startPixel, ui8 count)
{
	ui8& controll_bit = ProcessBusReadRef<ui16, ui8>(mk_controll_byte);
	if (HasBit(controll_bit, 0)) // BG Display enabled
	{

		ui16 tile_data = 0;
		ui16 background_memory = 0;
		// Is the memory location we are accessing signed?
		bool unsig = true;
		ui8 scrollY = ProcessBusReadRef<ui16, ui8>(0xFF42);
		ui8 scrollX = ProcessBusReadRef<ui16, ui8>(0xFF43);

		// which tile data are we using? 
		if (HasBit(controll_bit, 4))
		{
			tile_data = 0x8000;
		}
		else
		{
			tile_data = 0x8800;
			unsig = false;
		}

		if (HasBit(controll_bit, 3))
			background_memory = 0x9C00;
		else
			background_memory = 0x9800;

		// y_pos tells us which one of the 32 tiles the scan-line is currently drawing
		ui8 y_pos = scrollY + line;

		// which of the 8 vertical pixels of the current 
		// tile is the scanline on?
		ui16 tile_row = ((y_pos / 8) * 32);

		// time to start drawing the 160 horizontal pixels
		// for this scanline

		ui8 palette = ProcessBusReadRef<ui16, ui8>(mk_background_pallet_address);

		for (ui8 pixel = startPixel; pixel < startPixel + count; pixel++)
		{
			ui8 x_pos = pixel + scrollX;


			// Out of the 32 horizontal tiles, what one are we currently on
			ui8 tile_col = (x_pos / 8);
			ui8 tile_num;

			// get the tile identity number. This can be signed as well as unsigned
			ui16 tileAddrss = background_memory + tile_row + tile_col;
			if (unsig)
			{
				tile_num = ProcessBusReadRef<ui16, ui8>(tileAddrss);
			}
			else
			{
				tile_num = static_cast<i8>(ProcessBusReadRef<ui16, ui8>(tileAddrss));
				tile_num += 128;
			}

			// Is this tile identifier is in memory.
			ui16 tile_location = tile_data + (tile_num * 16);


			// find the correct vertical line we're on of the 
			// tile to get the tile data 
			//from in memory
			ui8 vline = y_pos % 8;
			vline *= 2; // each vertical line takes up two bytes of memory
			ui8 data1 = ProcessBusReadRef<ui16, ui8>(tile_location + vline);
			ui8 data2 = ProcessBusReadRef<ui16, ui8>(tile_location + vline + 1);

			// pixel 0 in the tile is it 7 of data 1 and data2.
			// Pixel 1 is bit 6 etc..
			int colour_bit = 7 - (x_pos % 8);





			// combine data 2 and data 1 to get the colour id for this pixel 
			// in the tile


			int colourNum = (ui8)(data2 >> colour_bit) & 1; // Get the set bit
			colourNum <<= 1;
			colourNum |= (ui8)(data1 >> colour_bit) & 1; // Get the set bit

			int pixel_index = pixel + (160 * line);

			m_back_buffer_color_cache[pixel_index] = colourNum & 0x03;


			ui8 color = (palette >> (colourNum * 2)) & 0x03;




			pixel_index *= 4;
			m_back_buffer[pixel_index] = m_back_buffer[pixel_index + 1] = m_back_buffer[pixel_index + 2] = (3 - color) * 64;

		}

	}
	else
	{
		for (int pixel = startPixel; pixel < startPixel + count; pixel++)
		{
			int pixel_index = pixel + 160 * line;
			m_back_buffer_color_cache[pixel_index] = 0;
		}
	}
}


void EmuGB::DrawWindow(int line)
{
	ui8& controll_bit = ProcessBusReadRef<ui16, ui8>(mk_controll_byte);

	if (!HasBit(controll_bit, 5))
	{
		return;
	}

	ui16 tile_data = 0;
	ui16 background_memory = 0;
	// Is the memory location we are accessing signed?
	bool unsig = true;

	ui8 windowY = ProcessBusReadRef<ui16, ui8>(0xFF4A);

	if ((windowY > 143) || (windowY > line))
		return;

	i8 windowX = ProcessBusReadRef<ui16, ui8>(0xFF4B) - 7;

	// which tile data are we using? 
	if (HasBit(controll_bit, 4))
	{
		tile_data = 0x8000;
	}
	else
	{
		tile_data = 0x8800;
		unsig = false;
	}

	// Which window memory
	if (HasBit(controll_bit, 6))
		background_memory = 0x9C00;
	else
		background_memory = 0x9800;




	// y_pos tells us which one of the 32 tiles the scan-line is currently drawing
	ui8 y_pos = m_WindowLine;




	// which of the 8 vertical pixels of the current 
	// tile is the scanline on?
	ui16 tile_row = (y_pos / 8) * 32;


	ui8 palette = ProcessBusReadRef<ui16, ui8>(0xFF47);


	// Loop for all 32 window tiles
	for (ui8 tileX = 0; tileX < 32; tileX++)
	{
		ui8 tile_num = 0;

		// get the tile identity number. This can be signed as well as unsigned
		ui16 tileAddrss = background_memory + tile_row + tileX;

		if (unsig)
		{
			tile_num = ProcessBusReadRef<ui16, ui8>(tileAddrss);
		}
		else
		{
			tile_num = static_cast<i8>(ProcessBusReadRef<ui16, ui8>(tileAddrss));
			tile_num += 128;
		}

		// Is this tile identifier is in memory.
		ui16 tile_location = tile_data + (tile_num * 16);



		// find the correct vertical line we're on of the 
		// tile to get the tile data 
		//from in memory
		ui8 vline = y_pos % 8;
		vline *= 2; // each vertical line takes up two bytes of memory
		ui8 data1 = ProcessBusReadRef<ui16, ui8>(tile_location + vline);
		ui8 data2 = ProcessBusReadRef<ui16, ui8>(tile_location + vline + 1);





		for (ui8 pixelX = 0; pixelX < 8; pixelX++)
		{
			ui16 windowXPos = (tileX * 8) + pixelX + windowX;
			if (windowXPos >= ScreenWidthEmu())
				continue;






			// combine data 2 and data 1 to get the colour id for this pixel 
			// in the tile


			int colourNum = (ui8)(data2 >> (7 - pixelX)) & 1; // Get the set bit
			colourNum <<= 1;
			colourNum |= (ui8)(data1 >> (7 - pixelX)) & 1; // Get the set bit


			int pixel_index = windowXPos + (160 * line);

			m_back_buffer_color_cache[pixel_index] = colourNum & 0x03;


			ui8 color = (palette >> (colourNum * 2)) & 0x03;




			pixel_index *= 4;
			m_back_buffer[pixel_index] = m_back_buffer[pixel_index + 1] = m_back_buffer[pixel_index + 2] = (3 - color) * 64;
		}

	}


	m_WindowLine++;
}

void EmuGB::KeyPressEmu(ConsoleKeys key)
{
	ClearBit(joypadActual, (ui8)key);
}

void EmuGB::KeyReleaseEmu(ConsoleKeys key)
{
	SetBit(joypadActual, (ui8)key);
}

bool EmuGB::IsKeyDownEmu(ConsoleKeys key)
{
	return !HasBit(joypadActual, (ui8)key);
}

void EmuGB::EnableDisplay()
{
	if (!HasBit(ProcessBusReadRef<ui16, ui8>(mk_controll_byte), 7)) // Screen dissabled
	{
		m_display_enable_delay = 244;
	}
}

void EmuGB::DissableDisplay()
{
	if (HasBit(ProcessBusReadRef<ui16, ui8>(mk_controll_byte), 7)) // Screen enabled
	{
		m_display_enable_delay = 244;

		
		ProcessBusReadRef<ui16, ui8>(mk_video_line_byte) = 0;

		ui8& status = ProcessBusReadRef<ui16, ui8>(mk_video_status);
		status &= 0x7C;
		m_lcd_enabled = false;
	}
}

void EmuGB::UpdateLCDStatus()
{
	ui8& status = ProcessBusReadRef<ui16, ui8>(mk_video_status);
	// Set mode to memory
	status = (status & 0xFC) | (m_display_mode & 0x3);
}

void EmuGB::CompareLYAndLYC()
{
	
	ui8& controll_bit = ProcessBusReadRef<ui16, ui8>(mk_controll_byte);
	bool isDisplayEnabled = HasBit(controll_bit, 7);

	if (isDisplayEnabled)
	{
		ui8& status = ProcessBusReadRef<ui16, ui8>(mk_video_status);
		ui8& lyc = ProcessBusReadRef<ui16, ui8>(mk_lyc);
		ui8& line = ProcessBusReadRef<ui16, ui8>(mk_video_line_byte);

		if (lyc == line)
		{
			SetBit(status, 2);
			if (HasBit(status, 6))
			{
				RequestInterupt(CPUInterupt::LCD);
			}
		}
		else
		{
			ClearBit(status, 2);
		}
	}
}

void EmuGB::DMATransfer(const ui8& data)
{
	// Sprite Data is at data * 100
	ui16 address = data << 8;
	for (ui16 i = 0; i < 0xA0; i++)
	{
		ProcessBus<MemoryAccessType::Write, ui16, ui8>(0xFE00 + i, ProcessBusReadRef<ui16, ui8>(address + i));
	}
}

void EmuGB::RequestInterupt(CPUInterupt interupt)
{
	ProcessBusReadRef<ui16, ui8>(mk_cpu_interupt_flag_address) |= 1 << (ui8)interupt;
}

void EmuGB::InitOPJumpTables()
{
	m_opCodes[0x00] = &EmuGB::Op00;
	m_opCodes[0x01] = &EmuGB::Op01;
	m_opCodes[0x02] = &EmuGB::Op02;
	m_opCodes[0x03] = &EmuGB::Op03;
	m_opCodes[0x04] = &EmuGB::Op04;
	m_opCodes[0x05] = &EmuGB::Op05;
	m_opCodes[0x06] = &EmuGB::Op06;
	m_opCodes[0x07] = &EmuGB::Op07;
	m_opCodes[0x08] = &EmuGB::Op08;
	m_opCodes[0x09] = &EmuGB::Op09;
	m_opCodes[0x0A] = &EmuGB::Op0A;
	m_opCodes[0x0B] = &EmuGB::Op0B;
	m_opCodes[0x0C] = &EmuGB::Op0C;
	m_opCodes[0x0D] = &EmuGB::Op0D;
	m_opCodes[0x0E] = &EmuGB::Op0E;
	m_opCodes[0x0F] = &EmuGB::Op0F;

	m_opCodes[0x10] = &EmuGB::Op10;
	m_opCodes[0x11] = &EmuGB::Op11;
	m_opCodes[0x12] = &EmuGB::Op12;
	m_opCodes[0x13] = &EmuGB::Op13;
	m_opCodes[0x14] = &EmuGB::Op14;
	m_opCodes[0x15] = &EmuGB::Op15;
	m_opCodes[0x16] = &EmuGB::Op16;
	m_opCodes[0x17] = &EmuGB::Op17;
	m_opCodes[0x18] = &EmuGB::Op18;
	m_opCodes[0x19] = &EmuGB::Op19;
	m_opCodes[0x1A] = &EmuGB::Op1A;
	m_opCodes[0x1B] = &EmuGB::Op1B;
	m_opCodes[0x1C] = &EmuGB::Op1C;
	m_opCodes[0x1D] = &EmuGB::Op1D;
	m_opCodes[0x1E] = &EmuGB::Op1E;
	m_opCodes[0x1F] = &EmuGB::Op1F;

	m_opCodes[0x20] = &EmuGB::Op20;
	m_opCodes[0x21] = &EmuGB::Op21;
	m_opCodes[0x22] = &EmuGB::Op22;
	m_opCodes[0x23] = &EmuGB::Op23;
	m_opCodes[0x24] = &EmuGB::Op24;
	m_opCodes[0x25] = &EmuGB::Op25;
	m_opCodes[0x26] = &EmuGB::Op26;
	m_opCodes[0x27] = &EmuGB::Op27;
	m_opCodes[0x28] = &EmuGB::Op28;
	m_opCodes[0x29] = &EmuGB::Op29;
	m_opCodes[0x2A] = &EmuGB::Op2A;
	m_opCodes[0x2B] = &EmuGB::Op2B;
	m_opCodes[0x2C] = &EmuGB::Op2C;
	m_opCodes[0x2D] = &EmuGB::Op2D;
	m_opCodes[0x2E] = &EmuGB::Op2E;
	m_opCodes[0x2F] = &EmuGB::Op2F;

	m_opCodes[0x30] = &EmuGB::Op30;
	m_opCodes[0x31] = &EmuGB::Op31;
	m_opCodes[0x32] = &EmuGB::Op32;
	m_opCodes[0x33] = &EmuGB::Op33;
	m_opCodes[0x34] = &EmuGB::Op34;
	m_opCodes[0x35] = &EmuGB::Op35;
	m_opCodes[0x36] = &EmuGB::Op36;
	m_opCodes[0x37] = &EmuGB::Op37;
	m_opCodes[0x38] = &EmuGB::Op38;
	m_opCodes[0x39] = &EmuGB::Op39;
	m_opCodes[0x3A] = &EmuGB::Op3A;
	m_opCodes[0x3B] = &EmuGB::Op3B;
	m_opCodes[0x3C] = &EmuGB::Op3C;
	m_opCodes[0x3D] = &EmuGB::Op3D;
	m_opCodes[0x3E] = &EmuGB::Op3E;
	m_opCodes[0x3F] = &EmuGB::Op3F;

	m_opCodes[0x40] = &EmuGB::Op40;
	m_opCodes[0x41] = &EmuGB::Op41;
	m_opCodes[0x42] = &EmuGB::Op42;
	m_opCodes[0x43] = &EmuGB::Op43;
	m_opCodes[0x44] = &EmuGB::Op44;
	m_opCodes[0x45] = &EmuGB::Op45;
	m_opCodes[0x46] = &EmuGB::Op46;
	m_opCodes[0x47] = &EmuGB::Op47;
	m_opCodes[0x48] = &EmuGB::Op48;
	m_opCodes[0x49] = &EmuGB::Op49;
	m_opCodes[0x4A] = &EmuGB::Op4A;
	m_opCodes[0x4B] = &EmuGB::Op4B;
	m_opCodes[0x4C] = &EmuGB::Op4C;
	m_opCodes[0x4D] = &EmuGB::Op4D;
	m_opCodes[0x4E] = &EmuGB::Op4E;
	m_opCodes[0x4F] = &EmuGB::Op4F;

	m_opCodes[0x50] = &EmuGB::Op50;
	m_opCodes[0x51] = &EmuGB::Op51;
	m_opCodes[0x52] = &EmuGB::Op52;
	m_opCodes[0x53] = &EmuGB::Op53;
	m_opCodes[0x54] = &EmuGB::Op54;
	m_opCodes[0x55] = &EmuGB::Op55;
	m_opCodes[0x56] = &EmuGB::Op56;
	m_opCodes[0x57] = &EmuGB::Op57;
	m_opCodes[0x58] = &EmuGB::Op58;
	m_opCodes[0x59] = &EmuGB::Op59;
	m_opCodes[0x5A] = &EmuGB::Op5A;
	m_opCodes[0x5B] = &EmuGB::Op5B;
	m_opCodes[0x5C] = &EmuGB::Op5C;
	m_opCodes[0x5D] = &EmuGB::Op5D;
	m_opCodes[0x5E] = &EmuGB::Op5E;
	m_opCodes[0x5F] = &EmuGB::Op5F;

	m_opCodes[0x60] = &EmuGB::Op60;
	m_opCodes[0x61] = &EmuGB::Op61;
	m_opCodes[0x62] = &EmuGB::Op62;
	m_opCodes[0x63] = &EmuGB::Op63;
	m_opCodes[0x64] = &EmuGB::Op64;
	m_opCodes[0x65] = &EmuGB::Op65;
	m_opCodes[0x66] = &EmuGB::Op66;
	m_opCodes[0x67] = &EmuGB::Op67;
	m_opCodes[0x68] = &EmuGB::Op68;
	m_opCodes[0x69] = &EmuGB::Op69;
	m_opCodes[0x6A] = &EmuGB::Op6A;
	m_opCodes[0x6B] = &EmuGB::Op6B;
	m_opCodes[0x6C] = &EmuGB::Op6C;
	m_opCodes[0x6D] = &EmuGB::Op6D;
	m_opCodes[0x6E] = &EmuGB::Op6E;
	m_opCodes[0x6F] = &EmuGB::Op6F;

	m_opCodes[0x70] = &EmuGB::Op70;
	m_opCodes[0x71] = &EmuGB::Op71;
	m_opCodes[0x72] = &EmuGB::Op72;
	m_opCodes[0x73] = &EmuGB::Op73;
	m_opCodes[0x74] = &EmuGB::Op74;
	m_opCodes[0x75] = &EmuGB::Op75;
	m_opCodes[0x76] = &EmuGB::Op76;
	m_opCodes[0x77] = &EmuGB::Op77;
	m_opCodes[0x78] = &EmuGB::Op78;
	m_opCodes[0x79] = &EmuGB::Op79;
	m_opCodes[0x7A] = &EmuGB::Op7A;
	m_opCodes[0x7B] = &EmuGB::Op7B;
	m_opCodes[0x7C] = &EmuGB::Op7C;
	m_opCodes[0x7D] = &EmuGB::Op7D;
	m_opCodes[0x7E] = &EmuGB::Op7E;
	m_opCodes[0x7F] = &EmuGB::Op7F;

	m_opCodes[0x80] = &EmuGB::Op80;
	m_opCodes[0x81] = &EmuGB::Op81;
	m_opCodes[0x82] = &EmuGB::Op82;
	m_opCodes[0x83] = &EmuGB::Op83;
	m_opCodes[0x84] = &EmuGB::Op84;
	m_opCodes[0x85] = &EmuGB::Op85;
	m_opCodes[0x86] = &EmuGB::Op86;
	m_opCodes[0x87] = &EmuGB::Op87;
	m_opCodes[0x88] = &EmuGB::Op88;
	m_opCodes[0x89] = &EmuGB::Op89;
	m_opCodes[0x8A] = &EmuGB::Op8A;
	m_opCodes[0x8B] = &EmuGB::Op8B;
	m_opCodes[0x8C] = &EmuGB::Op8C;
	m_opCodes[0x8D] = &EmuGB::Op8D;
	m_opCodes[0x8E] = &EmuGB::Op8E;
	m_opCodes[0x8F] = &EmuGB::Op8F;

	m_opCodes[0x90] = &EmuGB::Op90;
	m_opCodes[0x91] = &EmuGB::Op91;
	m_opCodes[0x92] = &EmuGB::Op92;
	m_opCodes[0x93] = &EmuGB::Op93;
	m_opCodes[0x94] = &EmuGB::Op94;
	m_opCodes[0x95] = &EmuGB::Op95;
	m_opCodes[0x96] = &EmuGB::Op96;
	m_opCodes[0x97] = &EmuGB::Op97;
	m_opCodes[0x98] = &EmuGB::Op98;
	m_opCodes[0x99] = &EmuGB::Op99;
	m_opCodes[0x9A] = &EmuGB::Op9A;
	m_opCodes[0x9B] = &EmuGB::Op9B;
	m_opCodes[0x9C] = &EmuGB::Op9C;
	m_opCodes[0x9D] = &EmuGB::Op9D;
	m_opCodes[0x9E] = &EmuGB::Op9E;
	m_opCodes[0x9F] = &EmuGB::Op9F;

	m_opCodes[0xA0] = &EmuGB::OpA0;
	m_opCodes[0xA1] = &EmuGB::OpA1;
	m_opCodes[0xA2] = &EmuGB::OpA2;
	m_opCodes[0xA3] = &EmuGB::OpA3;
	m_opCodes[0xA4] = &EmuGB::OpA4;
	m_opCodes[0xA5] = &EmuGB::OpA5;
	m_opCodes[0xA6] = &EmuGB::OpA6;
	m_opCodes[0xA7] = &EmuGB::OpA7;
	m_opCodes[0xA8] = &EmuGB::OpA8;
	m_opCodes[0xA9] = &EmuGB::OpA9;
	m_opCodes[0xAA] = &EmuGB::OpAA;
	m_opCodes[0xAB] = &EmuGB::OpAB;
	m_opCodes[0xAC] = &EmuGB::OpAC;
	m_opCodes[0xAD] = &EmuGB::OpAD;
	m_opCodes[0xAE] = &EmuGB::OpAE;
	m_opCodes[0xAF] = &EmuGB::OpAF;

	m_opCodes[0xB0] = &EmuGB::OpB0;
	m_opCodes[0xB1] = &EmuGB::OpB1;
	m_opCodes[0xB2] = &EmuGB::OpB2;
	m_opCodes[0xB3] = &EmuGB::OpB3;
	m_opCodes[0xB4] = &EmuGB::OpB4;
	m_opCodes[0xB5] = &EmuGB::OpB5;
	m_opCodes[0xB6] = &EmuGB::OpB6;
	m_opCodes[0xB7] = &EmuGB::OpB7;
	m_opCodes[0xB8] = &EmuGB::OpB8;
	m_opCodes[0xB9] = &EmuGB::OpB9;
	m_opCodes[0xBA] = &EmuGB::OpBA;
	m_opCodes[0xBB] = &EmuGB::OpBB;
	m_opCodes[0xBC] = &EmuGB::OpBC;
	m_opCodes[0xBD] = &EmuGB::OpBD;
	m_opCodes[0xBE] = &EmuGB::OpBE;
	m_opCodes[0xBF] = &EmuGB::OpBF;

	m_opCodes[0xC0] = &EmuGB::OpC0;
	m_opCodes[0xC1] = &EmuGB::OpC1;
	m_opCodes[0xC2] = &EmuGB::OpC2;
	m_opCodes[0xC3] = &EmuGB::OpC3;
	m_opCodes[0xC4] = &EmuGB::OpC4;
	m_opCodes[0xC5] = &EmuGB::OpC5;
	m_opCodes[0xC6] = &EmuGB::OpC6;
	m_opCodes[0xC7] = &EmuGB::OpC7;
	m_opCodes[0xC8] = &EmuGB::OpC8;
	m_opCodes[0xC9] = &EmuGB::OpC9;
	m_opCodes[0xCA] = &EmuGB::OpCA;
	//m_opCodes[0xCB] = &EmuGB::OpCB;
	m_opCodes[0xCC] = &EmuGB::OpCC;
	m_opCodes[0xCD] = &EmuGB::OpCD;
	m_opCodes[0xCE] = &EmuGB::OpCE;
	m_opCodes[0xCF] = &EmuGB::OpCF;

	m_opCodes[0xD0] = &EmuGB::OpD0;
	m_opCodes[0xD1] = &EmuGB::OpD1;
	m_opCodes[0xD2] = &EmuGB::OpD2;
	m_opCodes[0xD3] = &EmuGB::OpD3;
	m_opCodes[0xD4] = &EmuGB::OpD4;
	m_opCodes[0xD5] = &EmuGB::OpD5;
	m_opCodes[0xD6] = &EmuGB::OpD6;
	m_opCodes[0xD7] = &EmuGB::OpD7;
	m_opCodes[0xD8] = &EmuGB::OpD8;
	m_opCodes[0xD9] = &EmuGB::OpD9;
	m_opCodes[0xDA] = &EmuGB::OpDA;
	m_opCodes[0xDB] = &EmuGB::OpDB;
	m_opCodes[0xDC] = &EmuGB::OpDC;
	m_opCodes[0xDD] = &EmuGB::OpDD;
	m_opCodes[0xDE] = &EmuGB::OpDE;
	m_opCodes[0xDF] = &EmuGB::OpDF;

	m_opCodes[0xE0] = &EmuGB::OpE0;
	m_opCodes[0xE1] = &EmuGB::OpE1;
	m_opCodes[0xE2] = &EmuGB::OpE2;
	m_opCodes[0xE3] = &EmuGB::OpE3;
	m_opCodes[0xE4] = &EmuGB::OpE4;
	m_opCodes[0xE5] = &EmuGB::OpE5;
	m_opCodes[0xE6] = &EmuGB::OpE6;
	m_opCodes[0xE7] = &EmuGB::OpE7;
	m_opCodes[0xE8] = &EmuGB::OpE8;
	m_opCodes[0xE9] = &EmuGB::OpE9;
	m_opCodes[0xEA] = &EmuGB::OpEA;
	m_opCodes[0xEB] = &EmuGB::OpEB;
	m_opCodes[0xEC] = &EmuGB::OpEC;
	m_opCodes[0xED] = &EmuGB::OpED;
	m_opCodes[0xEE] = &EmuGB::OpEE;
	m_opCodes[0xEF] = &EmuGB::OpEF;

	m_opCodes[0xF0] = &EmuGB::OpF0;
	m_opCodes[0xF1] = &EmuGB::OpF1;
	m_opCodes[0xF2] = &EmuGB::OpF2;
	m_opCodes[0xF3] = &EmuGB::OpF3;
	m_opCodes[0xF4] = &EmuGB::OpF4;
	m_opCodes[0xF5] = &EmuGB::OpF5;
	m_opCodes[0xF6] = &EmuGB::OpF6;
	m_opCodes[0xF7] = &EmuGB::OpF7;
	m_opCodes[0xF8] = &EmuGB::OpF8;
	m_opCodes[0xF9] = &EmuGB::OpF9;
	m_opCodes[0xFA] = &EmuGB::OpFA;
	m_opCodes[0xFB] = &EmuGB::OpFB;
	m_opCodes[0xFC] = &EmuGB::OpFC;
	m_opCodes[0xFD] = &EmuGB::OpFD;
	m_opCodes[0xFE] = &EmuGB::OpFE;
	m_opCodes[0xFF] = &EmuGB::OpFF;


	m_CBOpCodes[0x00] = &EmuGB::OpCB00;
	m_CBOpCodes[0x01] = &EmuGB::OpCB01;
	m_CBOpCodes[0x02] = &EmuGB::OpCB02;
	m_CBOpCodes[0x03] = &EmuGB::OpCB03;
	m_CBOpCodes[0x04] = &EmuGB::OpCB04;
	m_CBOpCodes[0x05] = &EmuGB::OpCB05;
	m_CBOpCodes[0x06] = &EmuGB::OpCB06;
	m_CBOpCodes[0x07] = &EmuGB::OpCB07;
	m_CBOpCodes[0x08] = &EmuGB::OpCB08;
	m_CBOpCodes[0x09] = &EmuGB::OpCB09;
	m_CBOpCodes[0x0A] = &EmuGB::OpCB0A;
	m_CBOpCodes[0x0B] = &EmuGB::OpCB0B;
	m_CBOpCodes[0x0C] = &EmuGB::OpCB0C;
	m_CBOpCodes[0x0D] = &EmuGB::OpCB0D;
	m_CBOpCodes[0x0E] = &EmuGB::OpCB0E;
	m_CBOpCodes[0x0F] = &EmuGB::OpCB0F;

	m_CBOpCodes[0x10] = &EmuGB::OpCB10;
	m_CBOpCodes[0x11] = &EmuGB::OpCB11;
	m_CBOpCodes[0x12] = &EmuGB::OpCB12;
	m_CBOpCodes[0x13] = &EmuGB::OpCB13;
	m_CBOpCodes[0x14] = &EmuGB::OpCB14;
	m_CBOpCodes[0x15] = &EmuGB::OpCB15;
	m_CBOpCodes[0x16] = &EmuGB::OpCB16;
	m_CBOpCodes[0x17] = &EmuGB::OpCB17;
	m_CBOpCodes[0x18] = &EmuGB::OpCB18;
	m_CBOpCodes[0x19] = &EmuGB::OpCB19;
	m_CBOpCodes[0x1A] = &EmuGB::OpCB1A;
	m_CBOpCodes[0x1B] = &EmuGB::OpCB1B;
	m_CBOpCodes[0x1C] = &EmuGB::OpCB1C;
	m_CBOpCodes[0x1D] = &EmuGB::OpCB1D;
	m_CBOpCodes[0x1E] = &EmuGB::OpCB1E;
	m_CBOpCodes[0x1F] = &EmuGB::OpCB1F;

	m_CBOpCodes[0x20] = &EmuGB::OpCB20;
	m_CBOpCodes[0x21] = &EmuGB::OpCB21;
	m_CBOpCodes[0x22] = &EmuGB::OpCB22;
	m_CBOpCodes[0x23] = &EmuGB::OpCB23;
	m_CBOpCodes[0x24] = &EmuGB::OpCB24;
	m_CBOpCodes[0x25] = &EmuGB::OpCB25;
	m_CBOpCodes[0x26] = &EmuGB::OpCB26;
	m_CBOpCodes[0x27] = &EmuGB::OpCB27;
	m_CBOpCodes[0x28] = &EmuGB::OpCB28;
	m_CBOpCodes[0x29] = &EmuGB::OpCB29;
	m_CBOpCodes[0x2A] = &EmuGB::OpCB2A;
	m_CBOpCodes[0x2B] = &EmuGB::OpCB2B;
	m_CBOpCodes[0x2C] = &EmuGB::OpCB2C;
	m_CBOpCodes[0x2D] = &EmuGB::OpCB2D;
	m_CBOpCodes[0x2E] = &EmuGB::OpCB2E;
	m_CBOpCodes[0x2F] = &EmuGB::OpCB2F;

	m_CBOpCodes[0x30] = &EmuGB::OpCB30;
	m_CBOpCodes[0x31] = &EmuGB::OpCB31;
	m_CBOpCodes[0x32] = &EmuGB::OpCB32;
	m_CBOpCodes[0x33] = &EmuGB::OpCB33;
	m_CBOpCodes[0x34] = &EmuGB::OpCB34;
	m_CBOpCodes[0x35] = &EmuGB::OpCB35;
	m_CBOpCodes[0x36] = &EmuGB::OpCB36;
	m_CBOpCodes[0x37] = &EmuGB::OpCB37;
	m_CBOpCodes[0x38] = &EmuGB::OpCB38;
	m_CBOpCodes[0x39] = &EmuGB::OpCB39;
	m_CBOpCodes[0x3A] = &EmuGB::OpCB3A;
	m_CBOpCodes[0x3B] = &EmuGB::OpCB3B;
	m_CBOpCodes[0x3C] = &EmuGB::OpCB3C;
	m_CBOpCodes[0x3D] = &EmuGB::OpCB3D;
	m_CBOpCodes[0x3E] = &EmuGB::OpCB3E;
	m_CBOpCodes[0x3F] = &EmuGB::OpCB3F;

	m_CBOpCodes[0x40] = &EmuGB::OpCB40;
	m_CBOpCodes[0x41] = &EmuGB::OpCB41;
	m_CBOpCodes[0x42] = &EmuGB::OpCB42;
	m_CBOpCodes[0x43] = &EmuGB::OpCB43;
	m_CBOpCodes[0x44] = &EmuGB::OpCB44;
	m_CBOpCodes[0x45] = &EmuGB::OpCB45;
	m_CBOpCodes[0x46] = &EmuGB::OpCB46;
	m_CBOpCodes[0x47] = &EmuGB::OpCB47;
	m_CBOpCodes[0x48] = &EmuGB::OpCB48;
	m_CBOpCodes[0x49] = &EmuGB::OpCB49;
	m_CBOpCodes[0x4A] = &EmuGB::OpCB4A;
	m_CBOpCodes[0x4B] = &EmuGB::OpCB4B;
	m_CBOpCodes[0x4C] = &EmuGB::OpCB4C;
	m_CBOpCodes[0x4D] = &EmuGB::OpCB4D;
	m_CBOpCodes[0x4E] = &EmuGB::OpCB4E;
	m_CBOpCodes[0x4F] = &EmuGB::OpCB4F;

	m_CBOpCodes[0x50] = &EmuGB::OpCB50;
	m_CBOpCodes[0x51] = &EmuGB::OpCB51;
	m_CBOpCodes[0x52] = &EmuGB::OpCB52;
	m_CBOpCodes[0x53] = &EmuGB::OpCB53;
	m_CBOpCodes[0x54] = &EmuGB::OpCB54;
	m_CBOpCodes[0x55] = &EmuGB::OpCB55;
	m_CBOpCodes[0x56] = &EmuGB::OpCB56;
	m_CBOpCodes[0x57] = &EmuGB::OpCB57;
	m_CBOpCodes[0x58] = &EmuGB::OpCB58;
	m_CBOpCodes[0x59] = &EmuGB::OpCB59;
	m_CBOpCodes[0x5A] = &EmuGB::OpCB5A;
	m_CBOpCodes[0x5B] = &EmuGB::OpCB5B;
	m_CBOpCodes[0x5C] = &EmuGB::OpCB5C;
	m_CBOpCodes[0x5D] = &EmuGB::OpCB5D;
	m_CBOpCodes[0x5E] = &EmuGB::OpCB5E;
	m_CBOpCodes[0x5F] = &EmuGB::OpCB5F;

	m_CBOpCodes[0x60] = &EmuGB::OpCB60;
	m_CBOpCodes[0x61] = &EmuGB::OpCB61;
	m_CBOpCodes[0x62] = &EmuGB::OpCB62;
	m_CBOpCodes[0x63] = &EmuGB::OpCB63;
	m_CBOpCodes[0x64] = &EmuGB::OpCB64;
	m_CBOpCodes[0x65] = &EmuGB::OpCB65;
	m_CBOpCodes[0x66] = &EmuGB::OpCB66;
	m_CBOpCodes[0x67] = &EmuGB::OpCB67;
	m_CBOpCodes[0x68] = &EmuGB::OpCB68;
	m_CBOpCodes[0x69] = &EmuGB::OpCB69;
	m_CBOpCodes[0x6A] = &EmuGB::OpCB6A;
	m_CBOpCodes[0x6B] = &EmuGB::OpCB6B;
	m_CBOpCodes[0x6C] = &EmuGB::OpCB6C;
	m_CBOpCodes[0x6D] = &EmuGB::OpCB6D;
	m_CBOpCodes[0x6E] = &EmuGB::OpCB6E;
	m_CBOpCodes[0x6F] = &EmuGB::OpCB6F;

	m_CBOpCodes[0x70] = &EmuGB::OpCB70;
	m_CBOpCodes[0x71] = &EmuGB::OpCB71;
	m_CBOpCodes[0x72] = &EmuGB::OpCB72;
	m_CBOpCodes[0x73] = &EmuGB::OpCB73;
	m_CBOpCodes[0x74] = &EmuGB::OpCB74;
	m_CBOpCodes[0x75] = &EmuGB::OpCB75;
	m_CBOpCodes[0x76] = &EmuGB::OpCB76;
	m_CBOpCodes[0x77] = &EmuGB::OpCB77;
	m_CBOpCodes[0x78] = &EmuGB::OpCB78;
	m_CBOpCodes[0x79] = &EmuGB::OpCB79;
	m_CBOpCodes[0x7A] = &EmuGB::OpCB7A;
	m_CBOpCodes[0x7B] = &EmuGB::OpCB7B;
	m_CBOpCodes[0x7C] = &EmuGB::OpCB7C;
	m_CBOpCodes[0x7D] = &EmuGB::OpCB7D;
	m_CBOpCodes[0x7E] = &EmuGB::OpCB7E;
	m_CBOpCodes[0x7F] = &EmuGB::OpCB7F;

	m_CBOpCodes[0x80] = &EmuGB::OpCB80;
	m_CBOpCodes[0x81] = &EmuGB::OpCB81;
	m_CBOpCodes[0x82] = &EmuGB::OpCB82;
	m_CBOpCodes[0x83] = &EmuGB::OpCB83;
	m_CBOpCodes[0x84] = &EmuGB::OpCB84;
	m_CBOpCodes[0x85] = &EmuGB::OpCB85;
	m_CBOpCodes[0x86] = &EmuGB::OpCB86;
	m_CBOpCodes[0x87] = &EmuGB::OpCB87;
	m_CBOpCodes[0x88] = &EmuGB::OpCB88;
	m_CBOpCodes[0x89] = &EmuGB::OpCB89;
	m_CBOpCodes[0x8A] = &EmuGB::OpCB8A;
	m_CBOpCodes[0x8B] = &EmuGB::OpCB8B;
	m_CBOpCodes[0x8C] = &EmuGB::OpCB8C;
	m_CBOpCodes[0x8D] = &EmuGB::OpCB8D;
	m_CBOpCodes[0x8E] = &EmuGB::OpCB8E;
	m_CBOpCodes[0x8F] = &EmuGB::OpCB8F;

	m_CBOpCodes[0x90] = &EmuGB::OpCB90;
	m_CBOpCodes[0x91] = &EmuGB::OpCB91;
	m_CBOpCodes[0x92] = &EmuGB::OpCB92;
	m_CBOpCodes[0x93] = &EmuGB::OpCB93;
	m_CBOpCodes[0x94] = &EmuGB::OpCB94;
	m_CBOpCodes[0x95] = &EmuGB::OpCB95;
	m_CBOpCodes[0x96] = &EmuGB::OpCB96;
	m_CBOpCodes[0x97] = &EmuGB::OpCB97;
	m_CBOpCodes[0x98] = &EmuGB::OpCB98;
	m_CBOpCodes[0x99] = &EmuGB::OpCB99;
	m_CBOpCodes[0x9A] = &EmuGB::OpCB9A;
	m_CBOpCodes[0x9B] = &EmuGB::OpCB9B;
	m_CBOpCodes[0x9C] = &EmuGB::OpCB9C;
	m_CBOpCodes[0x9D] = &EmuGB::OpCB9D;
	m_CBOpCodes[0x9E] = &EmuGB::OpCB9E;
	m_CBOpCodes[0x9F] = &EmuGB::OpCB9F;

	m_CBOpCodes[0xA0] = &EmuGB::OpCBA0;
	m_CBOpCodes[0xA1] = &EmuGB::OpCBA1;
	m_CBOpCodes[0xA2] = &EmuGB::OpCBA2;
	m_CBOpCodes[0xA3] = &EmuGB::OpCBA3;
	m_CBOpCodes[0xA4] = &EmuGB::OpCBA4;
	m_CBOpCodes[0xA5] = &EmuGB::OpCBA5;
	m_CBOpCodes[0xA6] = &EmuGB::OpCBA6;
	m_CBOpCodes[0xA7] = &EmuGB::OpCBA7;
	m_CBOpCodes[0xA8] = &EmuGB::OpCBA8;
	m_CBOpCodes[0xA9] = &EmuGB::OpCBA9;
	m_CBOpCodes[0xAA] = &EmuGB::OpCBAA;
	m_CBOpCodes[0xAB] = &EmuGB::OpCBAB;
	m_CBOpCodes[0xAC] = &EmuGB::OpCBAC;
	m_CBOpCodes[0xAD] = &EmuGB::OpCBAD;
	m_CBOpCodes[0xAE] = &EmuGB::OpCBAE;
	m_CBOpCodes[0xAF] = &EmuGB::OpCBAF;

	m_CBOpCodes[0xB0] = &EmuGB::OpCBB0;
	m_CBOpCodes[0xB1] = &EmuGB::OpCBB1;
	m_CBOpCodes[0xB2] = &EmuGB::OpCBB2;
	m_CBOpCodes[0xB3] = &EmuGB::OpCBB3;
	m_CBOpCodes[0xB4] = &EmuGB::OpCBB4;
	m_CBOpCodes[0xB5] = &EmuGB::OpCBB5;
	m_CBOpCodes[0xB6] = &EmuGB::OpCBB6;
	m_CBOpCodes[0xB7] = &EmuGB::OpCBB7;
	m_CBOpCodes[0xB8] = &EmuGB::OpCBB8;
	m_CBOpCodes[0xB9] = &EmuGB::OpCBB9;
	m_CBOpCodes[0xBA] = &EmuGB::OpCBBA;
	m_CBOpCodes[0xBB] = &EmuGB::OpCBBB;
	m_CBOpCodes[0xBC] = &EmuGB::OpCBBC;
	m_CBOpCodes[0xBD] = &EmuGB::OpCBBD;
	m_CBOpCodes[0xBE] = &EmuGB::OpCBBE;
	m_CBOpCodes[0xBF] = &EmuGB::OpCBBF;

	m_CBOpCodes[0xC0] = &EmuGB::OpCBC0;
	m_CBOpCodes[0xC1] = &EmuGB::OpCBC1;
	m_CBOpCodes[0xC2] = &EmuGB::OpCBC2;
	m_CBOpCodes[0xC3] = &EmuGB::OpCBC3;
	m_CBOpCodes[0xC4] = &EmuGB::OpCBC4;
	m_CBOpCodes[0xC5] = &EmuGB::OpCBC5;
	m_CBOpCodes[0xC6] = &EmuGB::OpCBC6;
	m_CBOpCodes[0xC7] = &EmuGB::OpCBC7;
	m_CBOpCodes[0xC8] = &EmuGB::OpCBC8;
	m_CBOpCodes[0xC9] = &EmuGB::OpCBC9;
	m_CBOpCodes[0xCA] = &EmuGB::OpCBCA;
	m_CBOpCodes[0xCB] = &EmuGB::OpCBCB;
	m_CBOpCodes[0xCC] = &EmuGB::OpCBCC;
	m_CBOpCodes[0xCD] = &EmuGB::OpCBCD;
	m_CBOpCodes[0xCE] = &EmuGB::OpCBCE;
	m_CBOpCodes[0xCF] = &EmuGB::OpCBCF;

	m_CBOpCodes[0xD0] = &EmuGB::OpCBD0;
	m_CBOpCodes[0xD1] = &EmuGB::OpCBD1;
	m_CBOpCodes[0xD2] = &EmuGB::OpCBD2;
	m_CBOpCodes[0xD3] = &EmuGB::OpCBD3;
	m_CBOpCodes[0xD4] = &EmuGB::OpCBD4;
	m_CBOpCodes[0xD5] = &EmuGB::OpCBD5;
	m_CBOpCodes[0xD6] = &EmuGB::OpCBD6;
	m_CBOpCodes[0xD7] = &EmuGB::OpCBD7;
	m_CBOpCodes[0xD8] = &EmuGB::OpCBD8;
	m_CBOpCodes[0xD9] = &EmuGB::OpCBD9;
	m_CBOpCodes[0xDA] = &EmuGB::OpCBDA;
	m_CBOpCodes[0xDB] = &EmuGB::OpCBDB;
	m_CBOpCodes[0xDC] = &EmuGB::OpCBDC;
	m_CBOpCodes[0xDD] = &EmuGB::OpCBDD;
	m_CBOpCodes[0xDE] = &EmuGB::OpCBDE;
	m_CBOpCodes[0xDF] = &EmuGB::OpCBDF;

	m_CBOpCodes[0xE0] = &EmuGB::OpCBE0;
	m_CBOpCodes[0xE1] = &EmuGB::OpCBE1;
	m_CBOpCodes[0xE2] = &EmuGB::OpCBE2;
	m_CBOpCodes[0xE3] = &EmuGB::OpCBE3;
	m_CBOpCodes[0xE4] = &EmuGB::OpCBE4;
	m_CBOpCodes[0xE5] = &EmuGB::OpCBE5;
	m_CBOpCodes[0xE6] = &EmuGB::OpCBE6;
	m_CBOpCodes[0xE7] = &EmuGB::OpCBE7;
	m_CBOpCodes[0xE8] = &EmuGB::OpCBE8;
	m_CBOpCodes[0xE9] = &EmuGB::OpCBE9;
	m_CBOpCodes[0xEA] = &EmuGB::OpCBEA;
	m_CBOpCodes[0xEB] = &EmuGB::OpCBEB;
	m_CBOpCodes[0xEC] = &EmuGB::OpCBEC;
	m_CBOpCodes[0xED] = &EmuGB::OpCBED;
	m_CBOpCodes[0xEE] = &EmuGB::OpCBEE;
	m_CBOpCodes[0xEF] = &EmuGB::OpCBEF;

	m_CBOpCodes[0xF0] = &EmuGB::OpCBF0;
	m_CBOpCodes[0xF1] = &EmuGB::OpCBF1;
	m_CBOpCodes[0xF2] = &EmuGB::OpCBF2;
	m_CBOpCodes[0xF3] = &EmuGB::OpCBF3;
	m_CBOpCodes[0xF4] = &EmuGB::OpCBF4;
	m_CBOpCodes[0xF5] = &EmuGB::OpCBF5;
	m_CBOpCodes[0xF6] = &EmuGB::OpCBF6;
	m_CBOpCodes[0xF7] = &EmuGB::OpCBF7;
	m_CBOpCodes[0xF8] = &EmuGB::OpCBF8;
	m_CBOpCodes[0xF9] = &EmuGB::OpCBF9;
	m_CBOpCodes[0xFA] = &EmuGB::OpCBFA;
	m_CBOpCodes[0xFB] = &EmuGB::OpCBFB;
	m_CBOpCodes[0xFC] = &EmuGB::OpCBFC;
	m_CBOpCodes[0xFD] = &EmuGB::OpCBFD;
	m_CBOpCodes[0xFE] = &EmuGB::OpCBFE;
	m_CBOpCodes[0xFF] = &EmuGB::OpCBFF;
}

void EmuGB::Reset()
{


	for (unsigned int i = 0; i < m_display_buffer_size; i++)
	{
		m_front_buffer[i] = m_back_buffer[i] = 0x0;
	}



	for (int i = 0x0000; i < 0xFFFF; i++)
	{
		m_bus_memory[i] = 0x0;
	}

	m_cartridge.Reset();


	for (unsigned int i = 0; i < bootDMGSize; i++)
	{
		m_bus_memory[i] = bootDMG[i];
	}



	ProcessBusReadRef<ui16, ui8>(0xFF42) = 0;
	ProcessBusReadRef<ui16, ui8>(0xFF43) = 0;


	m_interrupts_enabled = false;
	ProcessBusReadRef<ui16, ui8>(mk_cpu_interupt_flag_address) = 0;

	// Reset the joypad
	joypadActual = 0xFF;
	ProcessBusReadRef<ui16, ui8>(0xFF00) = 0xFF;
	m_joypadCycles = 0;


	// Reset display
	ProcessBusReadRef<ui16, ui8>(mk_video_line_byte) = 144; // Set scanline to 144
	m_scanline_counter = 0;
	m_display_mode = 1;
	m_lcd_enabled = true;

	InitClockFrequency();



	// Reset registers
	GetWordRegister<EmuGB::WordRegisters::SP_REGISTER>() = 0x0000;
	GetWordRegister<EmuGB::WordRegisters::PC_REGISTER>() = 0x0000;
	GetWordRegister<EmuGB::WordRegisters::AF_REGISTER>() = 0x0000;
	GetWordRegister<EmuGB::WordRegisters::BC_REGISTER>() = 0x0000;
	GetWordRegister<EmuGB::WordRegisters::DE_REGISTER>() = 0x0000;
	GetWordRegister<EmuGB::WordRegisters::HL_REGISTER>() = 0x0000;



	m_cycle = 0;
	m_cycles = 0;
	m_IECycles = 0;
	m_timer_counter = 0;
	m_timer_frequancy = 0;
	m_devider_counter = 0;
}

bool EmuGB::InMemoryRange(ui16 start, ui16 end, ui16 address)
{
	return start <= address && end >= address;
}

unsigned int EmuGB::GetCycleModifier(unsigned int cycle)
{
	if (!cycle) return cycle;
	return cycle >> m_cycle_modifier;
}

ui8 EmuGB::ReadNextOPCode()
{
	ui8 result = m_bus_memory[GetWordRegister<WordRegisters::PC_REGISTER>()];

	Cost<4>(); // Each read takes 1m / 4t

	if (!m_halt_bug)
	{
		GetWordRegister<WordRegisters::PC_REGISTER>()++;
	}
	else
	{
		// If we have the halt bug, stay where we are and dont inc
		m_halt_bug = false;
	}
	return result;
}

ui8 EmuGB::ReadByteFromPC()
{
	ui8 d = ProcessBusReadRef<ui16,ui8>(GetWordRegister<EmuGB::WordRegisters::PC_REGISTER>()++);
	Cost<4>(); // Each read takes 1m / 4t
	return d;
}

i8 EmuGB::ReadSignedByteFromPC()
{
	i8 result = static_cast<i8>(m_bus_memory[GetWordRegister<EmuGB::WordRegisters::PC_REGISTER>()]);
	Cost<4>();
	GetWordRegister<EmuGB::WordRegisters::PC_REGISTER>()++;
	return result;
}

ui16 EmuGB::ReadWordFromPC()
{
	ui16 data = 0;

	ReadWordFromPC(data);

	return data;
}

void EmuGB::ReadWordFromPC(ui16& data)
{
	ProcessMemory<MemoryAccessType::Read, ui8, ui16, ui16>(m_bus_memory, GetWordRegister<EmuGB::WordRegisters::PC_REGISTER>(), data);

	// read cost
	Cost<8>();
	GetWordRegister<EmuGB::WordRegisters::PC_REGISTER>() += 2;
}

void EmuGB::SetPCRegister(const ui16& value)
{
	GetWordRegister<WordRegisters::PC_REGISTER>() = value;
	// Internal cost
	Cost<4>();
}

void EmuGB::InteruptCheck()
{
	ui8& interupt_flags = ProcessBusReadRef<ui16, ui8>(mk_cpu_interupt_flag_address);
	ui8& interupt_enabled_flags = ProcessBusReadRef<ui16, ui8>(mk_interrupt_enabled_flag_address);

	ui8 interuptsToProcess = interupt_flags & interupt_enabled_flags;


	if (interuptsToProcess > 0)
	{
		if (m_interrupts_enabled)
		{
			// Loop through for all possible interrupts 
			for (int i = 0; i < 5; i++)
			{
				if (HasBit(interuptsToProcess, (ui8)i))
				{
					m_interrupts_enabled = false; // Since we are now in a interrupt we need to disable future ones
					ClearBit(interupt_flags, (ui8)i);
					StackPush<WordRegisters::PC_REGISTER>();
					Cost<20>();

					switch (i)
					{
					case 0: // VBlank
						SetPCRegister(0x0040);
						//interupt_flags &= 0xFE;
						//Cost<4>();
						break;
					case 1: // LCDStat
						SetPCRegister(0x0048);
						//interupt_flags &= 0xFD;
						//Cost<4>();
						break;
					case 2: // Timer
						SetPCRegister(0x0050);
						//interupt_flags &= 0xFB;
						//Cost<4>();
						break;
					case 3: // Serial
						SetPCRegister(0x0058);
						//interupt_flags &= 0xF7;
						//Cost<4>();
						break;
					case 4: // Joypad
						SetPCRegister(0x0060);
						//interupt_flags &= 0xEF;
						//Cost<4>();
						break;
					default:
						assert(0 && "Unqnown interupt");
						break;
					}
					return;
				}
			}
		}
	}


}


void EmuGB::XOR(const ui8& value)
{
	ui8 result = GetByteRegister<ByteRegisters::A_REGISTER>() ^ value;

	GetByteRegister<ByteRegisters::A_REGISTER>() = result;

	GetByteRegister<ByteRegisters::F_REGISTER>() = 0x0;

	SetFlag<Flags::FLAG_ZERO>(result == 0);

}

void EmuGB::Jr()
{
	ui16 current = GetWordRegister<WordRegisters::PC_REGISTER>();
	ui16 new_pc = current + 1 + static_cast<i8>(ReadByteFromPC());
	SetPCRegister(new_pc);
}

void EmuGB::Jr(const ui16& address)
{
	SetPCRegister(address);
}


void EmuGB::rl(ui8& value, bool a_reg)
{
	ui8 carry = GetFlag<Flags::FLAG_CARRY>() ? 1 : 0;
	GetByteRegister<ByteRegisters::F_REGISTER>() = 0x0;
	if ((value & 0x80) != 0)
	{
		SetFlag<Flags::FLAG_CARRY, true>();
	}

	value <<= 1;

	value |= carry;
	if (!a_reg)
	{
		SetFlag<Flags::FLAG_ZERO>(value == 0);
	}
}

void EmuGB::Cp(const ui8& value)
{
	ui8 reg = GetByteRegister<ByteRegisters::A_REGISTER>();

	SetFlag<Flags::FLAG_SUBTRACT, true>();
	SetFlag<Flags::FLAG_CARRY>(reg < value);
	SetFlag<Flags::FLAG_ZERO>(reg == value);
	SetFlag<Flags::FLAG_HALF_CARRY>(((reg - value) & 0xF) > (reg & 0xF));

}

void EmuGB::Sub(const ui8& value)
{
	int current_register = GetByteRegister<ByteRegisters::A_REGISTER>();
	int result = current_register - value;
	int carrybits = current_register ^ value ^ result;

	GetByteRegister<ByteRegisters::A_REGISTER>() = static_cast<ui8>(result);

	GetByteRegister<ByteRegisters::F_REGISTER>() = 0x0;

	SetFlag<Flags::FLAG_SUBTRACT, true>();

	SetFlag<Flags::FLAG_ZERO>(static_cast<ui8>(result) == 0);

	if ((carrybits & 0x100) != 0)
	{
		SetFlag<Flags::FLAG_CARRY, true>();
	}
	if ((carrybits & 0x10) != 0)
	{
		SetFlag<Flags::FLAG_HALF_CARRY, true>();
	}
}


void EmuGB::OR(const ui8& value)
{
	ui8 or_val = GetByteRegister<ByteRegisters::A_REGISTER>();
	ui8 result = or_val | value;

	GetByteRegister<ByteRegisters::A_REGISTER>() = result;

	GetByteRegister<ByteRegisters::F_REGISTER>() = 0x0;

	SetFlag<Flags::FLAG_ZERO>(result == 0);
}

void EmuGB::AND(const ui8& value)
{
	ui8 result = GetByteRegister<ByteRegisters::A_REGISTER>() & value;

	GetByteRegister<ByteRegisters::A_REGISTER>() = result;

	GetByteRegister<ByteRegisters::F_REGISTER>() = 0x0;
	SetFlag<Flags::FLAG_ZERO>(result == 0);
	SetFlag<Flags::FLAG_HALF_CARRY,true>();
}

void EmuGB::Swap(ui8& value)
{
	ui8 low_half = value & 0x0F;
	ui8 high_half = (value >> 4) & 0x0F;

	value = (low_half << 4) + high_half;

	GetByteRegister<ByteRegisters::F_REGISTER>() = 0x0;

	SetFlag<Flags::FLAG_ZERO>(value == 0);
}

void EmuGB::SLA(ui8& value)
{
	GetByteRegister<ByteRegisters::F_REGISTER>() = 0x0;
	if ((value & 0x80) != 0)
	{
		SetFlag<Flags::FLAG_CARRY, true>();
	}

	value <<= 1;
	SetFlag< Flags::FLAG_ZERO>( value == 0);
}

void EmuGB::SBC(const ui8& value)
{
	ui8 a_reg = GetByteRegister<ByteRegisters::A_REGISTER>();

	int carry = GetFlag<Flags::FLAG_CARRY>() ? 1 : 0;
	int result = a_reg - value - carry;

	GetByteRegister<ByteRegisters::F_REGISTER>() = 0x0;

	SetFlag<Flags::FLAG_SUBTRACT, true>();
	SetFlag<Flags::FLAG_ZERO>(static_cast<ui8>(result) == 0);
	if (result < 0)
	{
		SetFlag<Flags::FLAG_CARRY, true>();
	}

	if (((a_reg & 0x0F) - (value & 0x0F) - carry) < 0)
	{
		SetFlag<Flags::FLAG_HALF_CARRY, true>();
	}

	GetByteRegister<ByteRegisters::A_REGISTER>() = static_cast<ui8> (result);
}

void EmuGB::AddHL(const ui16& value)
{
	// Internal cost
	Cost<4>();

	int result = GetWordRegister<WordRegisters::HL_REGISTER>() + value;

	bool hasZero = GetFlag<Flags::FLAG_ZERO>();

	GetByteRegister<ByteRegisters::F_REGISTER>() = 0x0;

	SetFlag<Flags::FLAG_ZERO>(hasZero);

	if (result & 0x10000)
	{
		SetFlag<Flags::FLAG_CARRY, true>();
	}

	if ((GetWordRegister<WordRegisters::HL_REGISTER>() ^ value ^ (result & 0xFFFF)) & 0x1000)
	{
		SetFlag<Flags::FLAG_HALF_CARRY, true>();
	}

	GetWordRegister<WordRegisters::HL_REGISTER>() = static_cast<ui16>(result);

}

void EmuGB::Bit(const ui8& value, ui8 bit)
{
	SetFlag<Flags::FLAG_ZERO>(((value >> bit) & 0x01) == 0);
	SetFlag <Flags::FLAG_HALF_CARRY, true>();
	SetFlag<Flags::FLAG_SUBTRACT, false>();
}

void EmuGB::ADC(const ui8& value)
{
	int carry = GetFlag<Flags::FLAG_CARRY>() ? 1 : 0;
	int result = GetByteRegister<ByteRegisters::A_REGISTER>() + value + carry;

	SetFlag<Flags::FLAG_ZERO>(static_cast<ui8> (result) == 0);
	SetFlag<Flags::FLAG_CARRY>(result > 0xFF);
	SetFlag<Flags::FLAG_HALF_CARRY>(((GetByteRegister<ByteRegisters::A_REGISTER>() & 0x0F) + (value & 0x0F) + carry) > 0x0F);
	SetFlag<Flags::FLAG_SUBTRACT, false>();

	GetByteRegister<ByteRegisters::A_REGISTER>() = static_cast<ui8> (result);
}

void EmuGB::RRC(ui8& value, bool a_reg)
{
	GetByteRegister<ByteRegisters::F_REGISTER>() = 0x0;
	if ((value & 0x01) != 0)
	{
		SetFlag<Flags::FLAG_CARRY, true>();
		value >>= 1;
		value |= 0x80;
	}
	else
	{
		value >>= 1;
	}

	if (!a_reg)
	{
		SetFlag<Flags::FLAG_ZERO>(value == 0);
	}
}

void EmuGB::RR(ui8& value, bool a_reg)
{
	ui8 carry = GetFlag<Flags::FLAG_CARRY>() ? 0x80 : 0x00;

	GetByteRegister<ByteRegisters::F_REGISTER>() = 0x0;
	if ((value & 0x01) != 0)
	{
		SetFlag<Flags::FLAG_CARRY, true>();
	}

	value >>= 1;
	value |= carry;

	if (!a_reg)
	{
		SetFlag<Flags::FLAG_ZERO>(value == 0);
	}
}

void EmuGB::SRL(ui8& value)
{
	GetByteRegister<ByteRegisters::F_REGISTER>() = 0x0;
	if ((value & 0x01) != 0)
	{
		SetFlag<Flags::FLAG_CARRY, true>();
	}
	value >>= 1;
	SetFlag<Flags::FLAG_ZERO>(value == 0);
}

void EmuGB::SRA(ui8& value)
{
	GetByteRegister<ByteRegisters::F_REGISTER>() = 0x0;
	if ((value & 0x01) != 0)
	{
		SetFlag<Flags::FLAG_CARRY, true>();
	}

	if ((value & 0x80) != 0)
	{
		value >>= 1;
		value |= 0x80;
	}
	else
	{
		value >>= 1;
	}

	SetFlag<Flags::FLAG_ZERO>(value == 0);
}

void EmuGB::RLC(ui8& value, bool a_reg)
{
	GetByteRegister<ByteRegisters::F_REGISTER>() = 0x0;
	if ((value & 0x80) != 0)
	{
		SetFlag<Flags::FLAG_CARRY, true>();
		value <<= 1;
		value |= 0x1;
	}
	else
	{
		value <<= 1;
	}
	if (!a_reg)
	{
		SetFlag<Flags::FLAG_ZERO>(value == 0);
	}
}

void EmuGB::Op00() {  }
void EmuGB::Op01() { GetWordRegister<WordRegisters::BC_REGISTER>() = ReadWordFromPC(); }
void EmuGB::Op02() { 
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::BC_REGISTER>(), GetByteRegister<ByteRegisters::A_REGISTER>());
	Cost<4>();
}
void EmuGB::Op03() { GetWordRegister<WordRegisters::BC_REGISTER>()++; Cost<4>(); }
void EmuGB::Op04() { INCByteRegister<ByteRegisters::B_REGISTER>(); }
void EmuGB::Op05() { DECByteRegister<ByteRegisters::B_REGISTER>(); }
void EmuGB::Op06() { GetByteRegister<ByteRegisters::B_REGISTER>() = ReadByteFromPC(); }
void EmuGB::Op07() { RLC(GetByteRegister<ByteRegisters::A_REGISTER>(), true); }
void EmuGB::Op08() {

	// Read cost 8
	ui16 pc = ReadWordFromPC();

	union
	{
		ui16 data;
		struct
		{
			ui8 low;
			ui8 high;
		}d;
	};
	data = GetWordRegister<WordRegisters::SP_REGISTER>();


	// Write Cost 8
	Cost<8>();
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(pc, d.low);
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(pc + 1, d.high);
}
void EmuGB::Op09() { AddHL(GetWordRegister<WordRegisters::BC_REGISTER>()); }
void EmuGB::Op0A() { 
	GetByteRegister<ByteRegisters::A_REGISTER>() = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::BC_REGISTER>()); 
	// Read Cost 4
	Cost<4>();
}
void EmuGB::Op0B() { 
	GetWordRegister<WordRegisters::BC_REGISTER>()--;
	// Internal cost
	Cost<4>();
}
void EmuGB::Op0C() { INCByteRegister<ByteRegisters::C_REGISTER>(); }
void EmuGB::Op0D() { DECByteRegister<ByteRegisters::C_REGISTER>(); }
void EmuGB::Op0E() { GetByteRegister<ByteRegisters::C_REGISTER>() = ReadByteFromPC(); }
void EmuGB::Op0F() { RRC(GetByteRegister<ByteRegisters::A_REGISTER>(), true); }

void EmuGB::Op10() { // STOP
	// Increment reg
	GetWordRegister<WordRegisters::PC_REGISTER>()++;

	if (m_cartridge.IsCB())
	{

		if (HasBit(ProcessBusReadRef<ui16, ui8>(0xFF4D), 0))
		{
			m_using_cb_speed = !m_using_cb_speed;
			if (m_using_cb_speed)
			{
				m_cycle_modifier = 1;
				ProcessBus<MemoryAccessType::Write, ui16, ui8>((ui16)0xFF4D, (ui8)0x80);
			}
			else
			{
				m_cycle_modifier = 0;
				ProcessBus<MemoryAccessType::Write, ui16, ui8>((ui16)0xFF4D, (ui8)0x00);
			}
		}
	}
}
void EmuGB::Op11() { GetWordRegister<WordRegisters::DE_REGISTER>() = ReadWordFromPC(); }
void EmuGB::Op12() { 
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::DE_REGISTER>(), GetByteRegister<ByteRegisters::A_REGISTER>()); 
	// Write cost
	Cost<4>();
}
void EmuGB::Op13() { 
	GetWordRegister<WordRegisters::DE_REGISTER>()++; 
	// Internal cost
	Cost<4>();
}
void EmuGB::Op14() { INCByteRegister<ByteRegisters::D_REGISTER>(); }
void EmuGB::Op15() { DECByteRegister<ByteRegisters::D_REGISTER>(); }
void EmuGB::Op16() { GetByteRegister<ByteRegisters::D_REGISTER>() = ReadByteFromPC(); }
void EmuGB::Op17() { rl(GetByteRegister<ByteRegisters::A_REGISTER>(), true); }
void EmuGB::Op18() { Jr(); }
void EmuGB::Op19() { AddHL(GetWordRegister<WordRegisters::DE_REGISTER>()); }
void EmuGB::Op1A() { 
	GetByteRegister<ByteRegisters::A_REGISTER>() = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::DE_REGISTER>()); 
	// Read cost
	Cost<4>();
}
void EmuGB::Op1B() { 
	GetWordRegister<WordRegisters::DE_REGISTER>()--;
	// Internal cost
	Cost<4>();
}
void EmuGB::Op1C() { INCByteRegister<ByteRegisters::E_REGISTER>(); }
void EmuGB::Op1D() { DECByteRegister<ByteRegisters::E_REGISTER>(); }
void EmuGB::Op1E() { GetByteRegister<ByteRegisters::E_REGISTER>() = ReadByteFromPC(); }
void EmuGB::Op1F() { RR(GetByteRegister<ByteRegisters::A_REGISTER>(), true); }

void EmuGB::Op20() {
	if (!GetFlag<Flags::FLAG_ZERO>())
		Jr();
	else
		ReadByteFromPC();
}
void EmuGB::Op21() { GetWordRegister<WordRegisters::HL_REGISTER>() = ReadWordFromPC(); }
void EmuGB::Op22() { 
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), GetByteRegister<ByteRegisters::A_REGISTER>());
	GetWordRegister<WordRegisters::HL_REGISTER>()++;
	// Write cost
	Cost<4>();
}
void EmuGB::Op23() {
	GetWordRegister<WordRegisters::HL_REGISTER>()++;
	// Internal cost
	Cost<4>();
}
void EmuGB::Op24() { INCByteRegister< ByteRegisters::H_REGISTER>(); }
void EmuGB::Op25() { DECByteRegister< ByteRegisters::H_REGISTER>(); }
void EmuGB::Op26() { GetByteRegister<ByteRegisters::H_REGISTER>() = ReadByteFromPC(); }
void EmuGB::Op27() {
	int a_reg = GetByteRegister<ByteRegisters::A_REGISTER>();

	if (!GetFlag<Flags::FLAG_SUBTRACT>())
	{
		if (GetFlag<Flags::FLAG_HALF_CARRY>() || ((a_reg & 0xF) > 9))
			a_reg += 0x06;

		if (GetFlag<Flags::FLAG_CARRY>() || (a_reg > 0x9F))
			a_reg += 0x60;
	}
	else
	{
		if (GetFlag<Flags::FLAG_HALF_CARRY>())
			a_reg = (a_reg - 6) & 0xFF;

		if (GetFlag<Flags::FLAG_CARRY>())
			a_reg -= 0x60;
	}
	SetFlag<Flags::FLAG_HALF_CARRY, false>();

	if ((a_reg & 0x100) == 0x100)
		SetFlag<Flags::FLAG_CARRY, true>();

	a_reg &= 0xFF;

	SetFlag<Flags::FLAG_ZERO>(a_reg == 0);

	GetByteRegister<ByteRegisters::A_REGISTER>() = (ui8)a_reg;
}
void EmuGB::Op28() {
	if (GetFlag<Flags::FLAG_ZERO>())
		Jr();
	else
		ReadByteFromPC();
}
void EmuGB::Op29() { AddHL(GetWordRegister<WordRegisters::HL_REGISTER>()); }
void EmuGB::Op2A() { 
	GetByteRegister<ByteRegisters::A_REGISTER>() = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	GetWordRegister<WordRegisters::HL_REGISTER>()++;
	// Read cost
	Cost<4>();
}
void EmuGB::Op2B() { 
	GetWordRegister<WordRegisters::HL_REGISTER>()--;
	// Internal cost
	Cost<4>();
}
void EmuGB::Op2C() { INCByteRegister<ByteRegisters::L_REGISTER>(); }
void EmuGB::Op2D() { DECByteRegister<ByteRegisters::L_REGISTER>(); }
void EmuGB::Op2E() { GetByteRegister<ByteRegisters::L_REGISTER>() = ReadByteFromPC(); }
void EmuGB::Op2F() { 
	GetByteRegister<ByteRegisters::A_REGISTER>() = ~GetByteRegister<ByteRegisters::A_REGISTER>(); // Flip all bits
	SetFlag<Flags::FLAG_HALF_CARRY>(true);
	SetFlag<Flags::FLAG_SUBTRACT>(true);
}

void EmuGB::Op30() {
	if (!GetFlag<Flags::FLAG_CARRY>())
	{
		Jr();
	}
	else
	{
		// Remove current byte from PC as this was the condition offset
		ReadByteFromPC();
	}
}
void EmuGB::Op31() { GetWordRegister<WordRegisters::SP_REGISTER>() = ReadWordFromPC(); }
void EmuGB::Op32() { 
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), GetByteRegister<ByteRegisters::A_REGISTER>()); 
	GetWordRegister<WordRegisters::HL_REGISTER>()--;
	// Write cost
	Cost<4>();
}
void EmuGB::Op33() { 
	GetWordRegister<WordRegisters::SP_REGISTER>()++;
	// Internal cost
	Cost<4>();
}
void EmuGB::Op34() { 
	ui8 data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	data++;
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), data);

	bool hasCarry = GetFlag<Flags::FLAG_CARRY>();

	GetByteRegister<ByteRegisters::F_REGISTER>() = 0;

	SetFlag<Flags::FLAG_CARRY>(hasCarry);
	SetFlag<Flags::FLAG_ZERO>(data == 0);
	SetFlag<Flags::FLAG_HALF_CARRY>((data & 0x0F) == 0x00);

	// Read and Write cost
	Cost<8>();
}
void EmuGB::Op35() {
	ui8 data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	data--;
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), data);

	if (!GetFlag<Flags::FLAG_CARRY>())
	{
		GetByteRegister<ByteRegisters::F_REGISTER>() = 0;
	}


	SetFlag<Flags::FLAG_ZERO>(data == 0);
	SetFlag<Flags::FLAG_SUBTRACT, true>();
	SetFlag<Flags::FLAG_HALF_CARRY>((data & 0x0F) == 0x0F);

	// Read and Write cost
	Cost<8>();
}
void EmuGB::Op36() { 
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), ReadByteFromPC()); 
	// Write cost
	Cost<4>();
}
void EmuGB::Op37() {
	SetFlag<Flags::FLAG_CARRY, true>();
	SetFlag<Flags::FLAG_HALF_CARRY, false >();
	SetFlag<Flags::FLAG_SUBTRACT, false >();
}
void EmuGB::Op38() {
	if (GetFlag<Flags::FLAG_CARRY>())
	{
		Jr();
	}
	else
	{
		// Remove current byte from PC as this was the condition offset
		ReadByteFromPC();
	}
}
void EmuGB::Op39() { AddHL(GetWordRegister<WordRegisters::SP_REGISTER>()); }
void EmuGB::Op3A() {
	GetByteRegister<ByteRegisters::A_REGISTER>() = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	GetWordRegister<WordRegisters::HL_REGISTER>()--;
	// Read cost
	Cost<4>();
}
void EmuGB::Op3B() { 
	GetWordRegister<WordRegisters::SP_REGISTER>()--;
	// Internal cost
	Cost<4>();
}
void EmuGB::Op3C() { INCByteRegister<ByteRegisters::A_REGISTER>(); }
void EmuGB::Op3D() { DECByteRegister<ByteRegisters::A_REGISTER>(); }
void EmuGB::Op3E() { GetByteRegister<ByteRegisters::A_REGISTER>() = ReadByteFromPC(); }
void EmuGB::Op3F() { // Flip Carry
	SetFlag<Flags::FLAG_CARRY>(!GetFlag<Flags::FLAG_CARRY>());
	SetFlag<Flags::FLAG_HALF_CARRY, false>();
	SetFlag<Flags::FLAG_SUBTRACT, false>();
}

void EmuGB::Op40() { GetByteRegister<ByteRegisters::B_REGISTER>() = GetByteRegister<ByteRegisters::B_REGISTER>(); }
void EmuGB::Op41() { GetByteRegister<ByteRegisters::B_REGISTER>() = GetByteRegister<ByteRegisters::C_REGISTER>(); }
void EmuGB::Op42() { GetByteRegister<ByteRegisters::B_REGISTER>() = GetByteRegister<ByteRegisters::D_REGISTER>(); }
void EmuGB::Op43() { GetByteRegister<ByteRegisters::B_REGISTER>() = GetByteRegister<ByteRegisters::E_REGISTER>(); }
void EmuGB::Op44() { GetByteRegister<ByteRegisters::B_REGISTER>() = GetByteRegister<ByteRegisters::H_REGISTER>(); }
void EmuGB::Op45() { GetByteRegister<ByteRegisters::B_REGISTER>() = GetByteRegister<ByteRegisters::L_REGISTER>(); }
void EmuGB::Op46() { 
	// Read cost
	Cost<4>();
	GetByteRegister<ByteRegisters::B_REGISTER>() = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>()); 
}
void EmuGB::Op47() { GetByteRegister<ByteRegisters::B_REGISTER>() = GetByteRegister<ByteRegisters::A_REGISTER>(); }
void EmuGB::Op48() { GetByteRegister<ByteRegisters::C_REGISTER>() = GetByteRegister<ByteRegisters::B_REGISTER>(); }
void EmuGB::Op49() { GetByteRegister<ByteRegisters::C_REGISTER>() = GetByteRegister<ByteRegisters::C_REGISTER>(); }
void EmuGB::Op4A() { GetByteRegister<ByteRegisters::C_REGISTER>() = GetByteRegister<ByteRegisters::D_REGISTER>(); }
void EmuGB::Op4B() { GetByteRegister<ByteRegisters::C_REGISTER>() = GetByteRegister<ByteRegisters::E_REGISTER>(); }
void EmuGB::Op4C() { GetByteRegister<ByteRegisters::C_REGISTER>() = GetByteRegister<ByteRegisters::H_REGISTER>(); }
void EmuGB::Op4D() { GetByteRegister<ByteRegisters::C_REGISTER>() = GetByteRegister<ByteRegisters::L_REGISTER>(); }
void EmuGB::Op4E() {
	// Read cost
	Cost<4>();
	GetByteRegister<ByteRegisters::C_REGISTER>() = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
}
void EmuGB::Op4F() { GetByteRegister<ByteRegisters::C_REGISTER>() = GetByteRegister<ByteRegisters::A_REGISTER>(); }

void EmuGB::Op50() { GetByteRegister<ByteRegisters::D_REGISTER>() = GetByteRegister<ByteRegisters::B_REGISTER>(); }
void EmuGB::Op51() { GetByteRegister<ByteRegisters::D_REGISTER>() = GetByteRegister<ByteRegisters::C_REGISTER>(); }
void EmuGB::Op52() { GetByteRegister<ByteRegisters::D_REGISTER>() = GetByteRegister<ByteRegisters::D_REGISTER>(); }
void EmuGB::Op53() { GetByteRegister<ByteRegisters::D_REGISTER>() = GetByteRegister<ByteRegisters::E_REGISTER>(); }
void EmuGB::Op54() { GetByteRegister<ByteRegisters::D_REGISTER>() = GetByteRegister<ByteRegisters::H_REGISTER>(); }
void EmuGB::Op55() { GetByteRegister<ByteRegisters::D_REGISTER>() = GetByteRegister<ByteRegisters::L_REGISTER>(); }
void EmuGB::Op56() {
	// Read cost
	Cost<4>();
	GetByteRegister<ByteRegisters::D_REGISTER>() = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>()); 
}
void EmuGB::Op57() { GetByteRegister<ByteRegisters::D_REGISTER>() = GetByteRegister<ByteRegisters::A_REGISTER>(); }
void EmuGB::Op58() { GetByteRegister<ByteRegisters::E_REGISTER>() = GetByteRegister<ByteRegisters::B_REGISTER>(); }
void EmuGB::Op59() { GetByteRegister<ByteRegisters::E_REGISTER>() = GetByteRegister<ByteRegisters::C_REGISTER>(); }
void EmuGB::Op5A() { GetByteRegister<ByteRegisters::E_REGISTER>() = GetByteRegister<ByteRegisters::D_REGISTER>(); }
void EmuGB::Op5B() { GetByteRegister<ByteRegisters::E_REGISTER>() = GetByteRegister<ByteRegisters::E_REGISTER>(); }
void EmuGB::Op5C() { GetByteRegister<ByteRegisters::E_REGISTER>() = GetByteRegister<ByteRegisters::H_REGISTER>(); }
void EmuGB::Op5D() { GetByteRegister<ByteRegisters::E_REGISTER>() = GetByteRegister<ByteRegisters::L_REGISTER>(); }
void EmuGB::Op5E() {
	// Read cost
	Cost<4>();
	GetByteRegister<ByteRegisters::E_REGISTER>() = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
}
void EmuGB::Op5F() { GetByteRegister<ByteRegisters::E_REGISTER>() = GetByteRegister<ByteRegisters::A_REGISTER>(); }

void EmuGB::Op60() { GetByteRegister<ByteRegisters::H_REGISTER>() = GetByteRegister<ByteRegisters::B_REGISTER>(); }
void EmuGB::Op61() { GetByteRegister<ByteRegisters::H_REGISTER>() = GetByteRegister<ByteRegisters::C_REGISTER>(); }
void EmuGB::Op62() { GetByteRegister<ByteRegisters::H_REGISTER>() = GetByteRegister<ByteRegisters::D_REGISTER>(); }
void EmuGB::Op63() { GetByteRegister<ByteRegisters::H_REGISTER>() = GetByteRegister<ByteRegisters::E_REGISTER>(); }
void EmuGB::Op64() { GetByteRegister<ByteRegisters::H_REGISTER>() = GetByteRegister<ByteRegisters::H_REGISTER>(); }
void EmuGB::Op65() { GetByteRegister<ByteRegisters::H_REGISTER>() = GetByteRegister<ByteRegisters::L_REGISTER>(); }
void EmuGB::Op66() {
	// Read cost
	Cost<4>();
	GetByteRegister<ByteRegisters::H_REGISTER>() = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
}
void EmuGB::Op67() { GetByteRegister<ByteRegisters::H_REGISTER>() = GetByteRegister<ByteRegisters::A_REGISTER>(); }
void EmuGB::Op68() { GetByteRegister<ByteRegisters::L_REGISTER>() = GetByteRegister<ByteRegisters::B_REGISTER>(); }
void EmuGB::Op69() { GetByteRegister<ByteRegisters::L_REGISTER>() = GetByteRegister<ByteRegisters::C_REGISTER>(); }
void EmuGB::Op6A() { GetByteRegister<ByteRegisters::L_REGISTER>() = GetByteRegister<ByteRegisters::D_REGISTER>(); }
void EmuGB::Op6B() { GetByteRegister<ByteRegisters::L_REGISTER>() = GetByteRegister<ByteRegisters::E_REGISTER>(); }
void EmuGB::Op6C() { GetByteRegister<ByteRegisters::L_REGISTER>() = GetByteRegister<ByteRegisters::H_REGISTER>(); }
void EmuGB::Op6D() { GetByteRegister<ByteRegisters::L_REGISTER>() = GetByteRegister<ByteRegisters::L_REGISTER>(); }
void EmuGB::Op6E() {
	// Read cost
	Cost<4>();
	GetByteRegister<ByteRegisters::L_REGISTER>() = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
}
void EmuGB::Op6F() { GetByteRegister<ByteRegisters::L_REGISTER>() = GetByteRegister<ByteRegisters::A_REGISTER>(); }

void EmuGB::Op70() { Cost<4>(); ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), GetByteRegister<ByteRegisters::B_REGISTER>()); }
void EmuGB::Op71() { Cost<4>(); ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), GetByteRegister<ByteRegisters::C_REGISTER>()); }
void EmuGB::Op72() { Cost<4>(); ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), GetByteRegister<ByteRegisters::D_REGISTER>()); }
void EmuGB::Op73() { Cost<4>(); ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), GetByteRegister<ByteRegisters::E_REGISTER>()); }
void EmuGB::Op74() { Cost<4>(); ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), GetByteRegister<ByteRegisters::H_REGISTER>()); }
void EmuGB::Op75() { Cost<4>(); ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), GetByteRegister<ByteRegisters::L_REGISTER>()); }
void EmuGB::Op76() { // If we are waitng on a EI, we trigger them before a halt
	if (m_IECycles > 0)
	{
		m_IECycles = 0;
		m_interrupts_enabled = true;
		GetWordRegister<WordRegisters::PC_REGISTER>()--;
	}
	else
	{
		m_halt = true;

		/* Check for halt bug */
		ui8 interruptEnabledFlag = ProcessBusReadRef<ui16, ui8>(mk_interrupt_enabled_flag_address);
		ui8 interruptFlag = ProcessBusReadRef<ui16, ui8>(mk_cpu_interupt_flag_address);

		// When interupts are dissabled HALT skips a pc instruction, we dont want this
		if (!m_interrupts_enabled && (interruptFlag & interruptEnabledFlag & 0x1F))
		{
			m_halt_bug = true;
		}
	}
}
void EmuGB::Op77() { Cost<4>(); ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), GetByteRegister<ByteRegisters::A_REGISTER>()); }
void EmuGB::Op78() { GetByteRegister<ByteRegisters::A_REGISTER>() = GetByteRegister<ByteRegisters::B_REGISTER>(); }
void EmuGB::Op79() { GetByteRegister<ByteRegisters::A_REGISTER>() = GetByteRegister<ByteRegisters::C_REGISTER>(); }
void EmuGB::Op7A() { GetByteRegister<ByteRegisters::A_REGISTER>() = GetByteRegister<ByteRegisters::D_REGISTER>(); }
void EmuGB::Op7B() { GetByteRegister<ByteRegisters::A_REGISTER>() = GetByteRegister<ByteRegisters::E_REGISTER>(); }
void EmuGB::Op7C() { GetByteRegister<ByteRegisters::A_REGISTER>() = GetByteRegister<ByteRegisters::H_REGISTER>(); }
void EmuGB::Op7D() { GetByteRegister<ByteRegisters::A_REGISTER>() = GetByteRegister<ByteRegisters::L_REGISTER>(); }
void EmuGB::Op7E() {
	// Read cost
	Cost<4>();
	GetByteRegister<ByteRegisters::A_REGISTER>() = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
}
void EmuGB::Op7F() { GetByteRegister<ByteRegisters::A_REGISTER>() = GetByteRegister<ByteRegisters::A_REGISTER>(); }

void EmuGB::Op80() { Add<ByteRegisters::A_REGISTER>(GetByteRegister<ByteRegisters::B_REGISTER>()); }
void EmuGB::Op81() { Add<ByteRegisters::A_REGISTER>(GetByteRegister<ByteRegisters::C_REGISTER>()); }
void EmuGB::Op82() { Add<ByteRegisters::A_REGISTER>(GetByteRegister<ByteRegisters::D_REGISTER>()); }
void EmuGB::Op83() { Add<ByteRegisters::A_REGISTER>(GetByteRegister<ByteRegisters::E_REGISTER>()); }
void EmuGB::Op84() { Add<ByteRegisters::A_REGISTER>(GetByteRegister<ByteRegisters::H_REGISTER>()); }
void EmuGB::Op85() { Add<ByteRegisters::A_REGISTER>(GetByteRegister<ByteRegisters::L_REGISTER>()); }
void EmuGB::Op86() { Cost<4>(); Add<ByteRegisters::A_REGISTER>(ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>())); }
void EmuGB::Op87() { Add<ByteRegisters::A_REGISTER>(GetByteRegister<ByteRegisters::A_REGISTER>()); }
void EmuGB::Op88() { ADC(GetByteRegister<ByteRegisters::B_REGISTER>()); }
void EmuGB::Op89() { ADC(GetByteRegister<ByteRegisters::C_REGISTER>()); }
void EmuGB::Op8A() { ADC(GetByteRegister<ByteRegisters::D_REGISTER>()); }
void EmuGB::Op8B() { ADC(GetByteRegister<ByteRegisters::E_REGISTER>()); }
void EmuGB::Op8C() { ADC(GetByteRegister<ByteRegisters::H_REGISTER>()); }
void EmuGB::Op8D() { ADC(GetByteRegister<ByteRegisters::L_REGISTER>()); }
void EmuGB::Op8E() { Cost<4>(); ADC(ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>())); }
void EmuGB::Op8F() { ADC(GetByteRegister<ByteRegisters::A_REGISTER>()); }

void EmuGB::Op90() { Sub(GetByteRegister<ByteRegisters::B_REGISTER>()); }
void EmuGB::Op91() { Sub(GetByteRegister<ByteRegisters::C_REGISTER>()); }
void EmuGB::Op92() { Sub(GetByteRegister<ByteRegisters::D_REGISTER>()); }
void EmuGB::Op93() { Sub(GetByteRegister<ByteRegisters::E_REGISTER>()); }
void EmuGB::Op94() { Sub(GetByteRegister<ByteRegisters::H_REGISTER>()); }
void EmuGB::Op95() { Sub(GetByteRegister<ByteRegisters::L_REGISTER>()); }
void EmuGB::Op96() { Cost<4>(); Sub(ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>())); }
void EmuGB::Op97() { Sub(GetByteRegister<ByteRegisters::A_REGISTER>()); }
void EmuGB::Op98() { SBC(GetByteRegister<ByteRegisters::B_REGISTER>()); }
void EmuGB::Op99() { SBC(GetByteRegister<ByteRegisters::C_REGISTER>()); }
void EmuGB::Op9A() { SBC(GetByteRegister<ByteRegisters::D_REGISTER>()); }
void EmuGB::Op9B() { SBC(GetByteRegister<ByteRegisters::E_REGISTER>()); }
void EmuGB::Op9C() { SBC(GetByteRegister<ByteRegisters::H_REGISTER>()); }
void EmuGB::Op9D() { SBC(GetByteRegister<ByteRegisters::L_REGISTER>()); }
void EmuGB::Op9E() { Cost<4>(); SBC(ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>())); }
void EmuGB::Op9F() { SBC(GetByteRegister<ByteRegisters::A_REGISTER>()); }

void EmuGB::OpA0() { AND(GetByteRegister<ByteRegisters::B_REGISTER>()); }
void EmuGB::OpA1() { AND(GetByteRegister<ByteRegisters::C_REGISTER>()); }
void EmuGB::OpA2() { AND(GetByteRegister<ByteRegisters::D_REGISTER>()); }
void EmuGB::OpA3() { AND(GetByteRegister<ByteRegisters::E_REGISTER>()); }
void EmuGB::OpA4() { AND(GetByteRegister<ByteRegisters::H_REGISTER>()); }
void EmuGB::OpA5() { AND(GetByteRegister<ByteRegisters::L_REGISTER>()); }
void EmuGB::OpA6() { Cost<4>(); AND(ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>())); }
void EmuGB::OpA7() { AND(GetByteRegister<ByteRegisters::A_REGISTER>()); }
void EmuGB::OpA8() { XOR(GetByteRegister<EmuGB::ByteRegisters::B_REGISTER>()); }
void EmuGB::OpA9() { XOR(GetByteRegister<EmuGB::ByteRegisters::C_REGISTER>()); }
void EmuGB::OpAA() { XOR(GetByteRegister<EmuGB::ByteRegisters::D_REGISTER>()); }
void EmuGB::OpAB() { XOR(GetByteRegister<EmuGB::ByteRegisters::E_REGISTER>()); }
void EmuGB::OpAC() { XOR(GetByteRegister<EmuGB::ByteRegisters::H_REGISTER>()); }
void EmuGB::OpAD() { XOR(GetByteRegister<EmuGB::ByteRegisters::L_REGISTER>()); }
void EmuGB::OpAE() { Cost<4>(); XOR(ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>())); }
void EmuGB::OpAF() { XOR(GetByteRegister<EmuGB::ByteRegisters::A_REGISTER>()); }

void EmuGB::OpB0() { OR(GetByteRegister<ByteRegisters::B_REGISTER>()); }
void EmuGB::OpB1() { OR(GetByteRegister<ByteRegisters::C_REGISTER>()); }
void EmuGB::OpB2() { OR(GetByteRegister<ByteRegisters::D_REGISTER>()); }
void EmuGB::OpB3() { OR(GetByteRegister<ByteRegisters::E_REGISTER>()); }
void EmuGB::OpB4() { OR(GetByteRegister<ByteRegisters::H_REGISTER>()); }
void EmuGB::OpB5() { OR(GetByteRegister<ByteRegisters::L_REGISTER>()); }
void EmuGB::OpB6() { Cost<4>(); OR(ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>())); }
void EmuGB::OpB7() { OR(GetByteRegister<ByteRegisters::A_REGISTER>()); }
void EmuGB::OpB8() { Cp(GetByteRegister<ByteRegisters::B_REGISTER>()); }
void EmuGB::OpB9() { Cp(GetByteRegister<ByteRegisters::C_REGISTER>()); }
void EmuGB::OpBA() { Cp(GetByteRegister<ByteRegisters::D_REGISTER>()); }
void EmuGB::OpBB() { Cp(GetByteRegister<ByteRegisters::E_REGISTER>()); }
void EmuGB::OpBC() { Cp(GetByteRegister<ByteRegisters::H_REGISTER>()); }
void EmuGB::OpBD() { Cp(GetByteRegister<ByteRegisters::L_REGISTER>()); }
void EmuGB::OpBE() { Cost<4>(); Cp(ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>())); }
void EmuGB::OpBF() { Cp(GetByteRegister<ByteRegisters::A_REGISTER>()); }

void EmuGB::OpC0() {
	if (!GetFlag<Flags::FLAG_ZERO>())
	{
		StackPop<WordRegisters::PC_REGISTER>();
	}
	// Internal Cost
	Cost<4>();
}
void EmuGB::OpC1() { StackPop<WordRegisters::BC_REGISTER>(); }
void EmuGB::OpC2() {
	ui16 address = ReadWordFromPC();
	if (!GetFlag<Flags::FLAG_ZERO>())
	{
		Jr(address);
	}
}
void EmuGB::OpC3() {
	SetPCRegister(ReadWordFromPC());
}
void EmuGB::OpC4() {
	ui16 word = ReadWordFromPC();
	if (!GetFlag<Flags::FLAG_ZERO>())
	{
		StackPush<WordRegisters::PC_REGISTER>();
		SetPCRegister(word);
	}
}
void EmuGB::OpC5() { 
	StackPush<WordRegisters::BC_REGISTER>();
	// Internal Cost
	Cost<4>();
}
void EmuGB::OpC6() { Add<ByteRegisters::A_REGISTER>(ReadByteFromPC()); }
void EmuGB::OpC7() {
	StackPush<WordRegisters::PC_REGISTER>();
	SetPCRegister(RESET_00);
}
void EmuGB::OpC8() { 
	if (GetFlag<Flags::FLAG_ZERO>())
	{
		StackPop<WordRegisters::PC_REGISTER>();
	}
	// Internal Cost
	Cost<4>();
}
void EmuGB::OpC9() { StackPop<WordRegisters::PC_REGISTER>(); }
void EmuGB::OpCA() {
	ui16 address = ReadWordFromPC();
	if (GetFlag<Flags::FLAG_ZERO>())
	{
		Jr(address);
	}
}
//void EmuGB::OpCB() { assert("Missing" && 0); }
void EmuGB::OpCC() {
	ui16 word = ReadWordFromPC();
	if (GetFlag<Flags::FLAG_ZERO>())
	{
		StackPush<WordRegisters::PC_REGISTER>();
		SetPCRegister(word);
	}
}
void EmuGB::OpCD() {
	ui16 newAddress = ReadWordFromPC();
	StackPush<WordRegisters::PC_REGISTER>();
	SetPCRegister(newAddress);
}
void EmuGB::OpCE() { ADC(ReadByteFromPC()); }
void EmuGB::OpCF() {
	StackPush<WordRegisters::PC_REGISTER>();
	SetPCRegister(RESET_08);
}

void EmuGB::OpD0() {
	if (!GetFlag<Flags::FLAG_CARRY>())
	{
		StackPop<WordRegisters::PC_REGISTER>();
	}
	// Internal Cost
	Cost<4>();
}
void EmuGB::OpD1() { StackPop<WordRegisters::DE_REGISTER>(); }
void EmuGB::OpD2() {
	ui16 address = ReadWordFromPC();
	if (!GetFlag<Flags::FLAG_CARRY>())
	{
		Jr(address);
	}
}
void EmuGB::OpD3() {  }
void EmuGB::OpD4() {
	ui16 word = ReadWordFromPC();
	if (!GetFlag<Flags::FLAG_CARRY>())
	{
		StackPush<WordRegisters::PC_REGISTER>();
		SetPCRegister(word);
	}
}
void EmuGB::OpD5() {
	StackPush<WordRegisters::DE_REGISTER>();
	// Internal cost
	Cost<4>();
}
void EmuGB::OpD6() { Sub(ReadByteFromPC()); }
void EmuGB::OpD7() {
	StackPush<WordRegisters::PC_REGISTER>();
	SetPCRegister(RESET_10);
}
void EmuGB::OpD8() {
	if (GetFlag<Flags::FLAG_CARRY>())
	{
		StackPop<WordRegisters::PC_REGISTER>();
	}
	// Internal Cost
	Cost<4>();
}
void EmuGB::OpD9() {
	StackPop<WordRegisters::PC_REGISTER>();
	m_interrupts_enabled = true;
}
void EmuGB::OpDA() {
	ui16 address = ReadWordFromPC();
	if (GetFlag<Flags::FLAG_CARRY>())
	{
		Jr(address);
	}
}
void EmuGB::OpDB() { }
void EmuGB::OpDC() {
	ui16 word = ReadWordFromPC();
	if (GetFlag<Flags::FLAG_CARRY>())
	{
		StackPush<WordRegisters::PC_REGISTER>();
		SetPCRegister(word);
	}
}
void EmuGB::OpDD() {  }
void EmuGB::OpDE() { SBC(ReadByteFromPC()); }
void EmuGB::OpDF() {
	StackPush<WordRegisters::PC_REGISTER>();
	SetPCRegister(RESET_18);
}

void EmuGB::OpE0() {
	Cost<4>();
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(static_cast<ui16> (0xFF00 + ReadByteFromPC()), GetByteRegister<ByteRegisters::A_REGISTER>());
}
void EmuGB::OpE1() { StackPop<WordRegisters::HL_REGISTER>(); }
void EmuGB::OpE2() {
	Cost<4>(); 
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(static_cast<ui16> (0xFF00 + GetByteRegister<ByteRegisters::C_REGISTER>()), GetByteRegister<ByteRegisters::A_REGISTER>()); 
}
void EmuGB::OpE3() {  }
void EmuGB::OpE4() {  }
void EmuGB::OpE5() {
	Cost<4>();
	StackPush<WordRegisters::HL_REGISTER>();
}
void EmuGB::OpE6() { AND(ReadByteFromPC()); }
void EmuGB::OpE7() {
	StackPush<WordRegisters::PC_REGISTER>();
	SetPCRegister(RESET_20);
}
void EmuGB::OpE8() {
	i8 value = ReadSignedByteFromPC();

	int result = GetWordRegister<WordRegisters::SP_REGISTER>() + value;
	
	GetByteRegister<ByteRegisters::F_REGISTER>() = 0;

	if (((GetWordRegister<WordRegisters::SP_REGISTER>() ^ value ^ (result & 0xFFFF)) & 0x100) == 0x100)
	{
		SetFlag<Flags::FLAG_CARRY, true>();
	}
	if (((GetWordRegister<WordRegisters::SP_REGISTER>() ^ value ^ (result & 0xFFFF)) & 0x10) == 0x10)
	{
		SetFlag<Flags::FLAG_HALF_CARRY, true>();
	}

	GetWordRegister<WordRegisters::SP_REGISTER>() = static_cast<ui16>(result);

	// Read and Write
	Cost<8>();
}
void EmuGB::OpE9() { 
	// Says there is no cost here, will have to belive them
	GetWordRegister<WordRegisters::PC_REGISTER>() = GetWordRegister<WordRegisters::HL_REGISTER>(); 
}
void EmuGB::OpEA() {
	Cost<4>(); 
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(ReadWordFromPC(), GetByteRegister<ByteRegisters::A_REGISTER>()); 
}
void EmuGB::OpEB() {  }
void EmuGB::OpEC() {  }
void EmuGB::OpED() {  }
void EmuGB::OpEE() { XOR(ReadByteFromPC()); }
void EmuGB::OpEF() {
	StackPush<WordRegisters::PC_REGISTER>();
	SetPCRegister(RESET_28);
}

void EmuGB::OpF0() {
	Cost<4>(); 
	GetByteRegister<ByteRegisters::A_REGISTER>() = ProcessBusReadRef<ui16, ui8>(static_cast<ui16> (0xFF00 + ReadByteFromPC())); 
}
void EmuGB::OpF1() {
	StackPop<WordRegisters::AF_REGISTER>();
	GetByteRegister<ByteRegisters::F_REGISTER>() &= 0xF0;
}
void EmuGB::OpF2() {
	Cost<4>(); 
	GetByteRegister<ByteRegisters::A_REGISTER>() = ProcessBusReadRef<ui16, ui8>(static_cast<ui16> (0xFF00 + GetByteRegister<ByteRegisters::C_REGISTER>()));
}
void EmuGB::OpF3() {
	m_interrupts_enabled = false;
	m_IECycles = 0;
}
void EmuGB::OpF4() {  }
void EmuGB::OpF5() {
	Cost<4>(); 
	StackPush<WordRegisters::AF_REGISTER>();
}
void EmuGB::OpF6() { OR(ReadByteFromPC()); }
void EmuGB::OpF7() {
	StackPush<WordRegisters::PC_REGISTER>();
	SetPCRegister(RESET_30);
}
void EmuGB::OpF8() {
	i8 n = ReadSignedByteFromPC();
	ui16 result = GetWordRegister<WordRegisters::SP_REGISTER>() + n;

	GetByteRegister<ByteRegisters::F_REGISTER>() = 0x0;

	if (((GetWordRegister<WordRegisters::SP_REGISTER>() ^ n ^ result) & 0x100) == 0x100)
	{
		SetFlag<Flags::FLAG_CARRY, true>();
	}
	if ((((GetWordRegister<WordRegisters::SP_REGISTER>() ^ n ^ result) & 0x10) == 0x10))
	{
		SetFlag<Flags::FLAG_HALF_CARRY, true>();
	}
	GetWordRegister<WordRegisters::HL_REGISTER>() = result;

	Cost<4>();
}
void EmuGB::OpF9() {
	Cost<4>();
	GetWordRegister<WordRegisters::SP_REGISTER>() = GetWordRegister<WordRegisters::HL_REGISTER>();
}
void EmuGB::OpFA() {
	Cost<4>();
	GetByteRegister<ByteRegisters::A_REGISTER>() = ProcessBusReadRef<ui16, ui8>(ReadWordFromPC());
}
void EmuGB::OpFB() {
	//m_interrupts_enabled = true;
	m_IECycles = GetCycleModifier(4) + 1;
}
void EmuGB::OpFC() {  }
void EmuGB::OpFD() {  }
void EmuGB::OpFE() { Cp(ReadByteFromPC()); }
void EmuGB::OpFF() {
	StackPush<WordRegisters::PC_REGISTER>();
	SetPCRegister(RESET_38);
}


void EmuGB::OpCB00() { RLC(GetByteRegister<ByteRegisters::B_REGISTER>()); }
void EmuGB::OpCB01() { RLC(GetByteRegister<ByteRegisters::C_REGISTER>()); }
void EmuGB::OpCB02() { RLC(GetByteRegister<ByteRegisters::D_REGISTER>()); }
void EmuGB::OpCB03() { RLC(GetByteRegister<ByteRegisters::E_REGISTER>()); }
void EmuGB::OpCB04() { RLC(GetByteRegister<ByteRegisters::H_REGISTER>()); }
void EmuGB::OpCB05() { RLC(GetByteRegister<ByteRegisters::L_REGISTER>()); }
void EmuGB::OpCB06() {
	ui8 data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	RLC(data);
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), data);
	Cost<8>();
}
void EmuGB::OpCB07() { RLC(GetByteRegister<ByteRegisters::A_REGISTER>()); }
void EmuGB::OpCB08() { RRC(GetByteRegister<ByteRegisters::B_REGISTER>()); }
void EmuGB::OpCB09() { RRC(GetByteRegister<ByteRegisters::C_REGISTER>()); }
void EmuGB::OpCB0A() { RRC(GetByteRegister<ByteRegisters::D_REGISTER>()); }
void EmuGB::OpCB0B() { RRC(GetByteRegister<ByteRegisters::E_REGISTER>()); }
void EmuGB::OpCB0C() { RRC(GetByteRegister<ByteRegisters::H_REGISTER>()); }
void EmuGB::OpCB0D() { RRC(GetByteRegister<ByteRegisters::L_REGISTER>()); }
void EmuGB::OpCB0E() {
	ui8 data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	RRC(data);
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), data);
	Cost<8>();
}
void EmuGB::OpCB0F() { RRC(GetByteRegister<ByteRegisters::A_REGISTER>()); }

void EmuGB::OpCB10() { rl(GetByteRegister<ByteRegisters::B_REGISTER>()); }
void EmuGB::OpCB11() { rl(GetByteRegister<ByteRegisters::C_REGISTER>()); }
void EmuGB::OpCB12() { rl(GetByteRegister<ByteRegisters::D_REGISTER>()); }
void EmuGB::OpCB13() { rl(GetByteRegister<ByteRegisters::E_REGISTER>()); }
void EmuGB::OpCB14() { rl(GetByteRegister<ByteRegisters::H_REGISTER>()); }
void EmuGB::OpCB15() { rl(GetByteRegister<ByteRegisters::L_REGISTER>()); }
void EmuGB::OpCB16() { 
	rl(ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>()));
	Cost<8>();
}
void EmuGB::OpCB17() { rl(GetByteRegister<ByteRegisters::A_REGISTER>(), false); }
void EmuGB::OpCB18() { RR(GetByteRegister<ByteRegisters::B_REGISTER>()); }
void EmuGB::OpCB19() { RR(GetByteRegister<ByteRegisters::C_REGISTER>()); }
void EmuGB::OpCB1A() { RR(GetByteRegister<ByteRegisters::D_REGISTER>()); }
void EmuGB::OpCB1B() { RR(GetByteRegister<ByteRegisters::E_REGISTER>()); }
void EmuGB::OpCB1C() { RR(GetByteRegister<ByteRegisters::H_REGISTER>()); }
void EmuGB::OpCB1D() { RR(GetByteRegister<ByteRegisters::L_REGISTER>()); }
void EmuGB::OpCB1E() {
	RR(ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>()));
	Cost<8>();
}
void EmuGB::OpCB1F() { RR(GetByteRegister<ByteRegisters::A_REGISTER>()); }

void EmuGB::OpCB20() { SLA(GetByteRegister< ByteRegisters::B_REGISTER>()); }
void EmuGB::OpCB21() { SLA(GetByteRegister< ByteRegisters::C_REGISTER>()); }
void EmuGB::OpCB22() { SLA(GetByteRegister< ByteRegisters::D_REGISTER>()); }
void EmuGB::OpCB23() { SLA(GetByteRegister< ByteRegisters::E_REGISTER>()); }
void EmuGB::OpCB24() { SLA(GetByteRegister< ByteRegisters::H_REGISTER>()); }
void EmuGB::OpCB25() { SLA(GetByteRegister< ByteRegisters::L_REGISTER>()); }
void EmuGB::OpCB26() {
	SLA(ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>()));
	Cost<8>();
}
void EmuGB::OpCB27() { SLA(GetByteRegister< ByteRegisters::A_REGISTER>()); }
void EmuGB::OpCB28() { SRA(GetByteRegister<ByteRegisters::B_REGISTER>()); }
void EmuGB::OpCB29() { SRA(GetByteRegister<ByteRegisters::C_REGISTER>()); }
void EmuGB::OpCB2A() { SRA(GetByteRegister<ByteRegisters::D_REGISTER>()); }
void EmuGB::OpCB2B() { SRA(GetByteRegister<ByteRegisters::E_REGISTER>()); }
void EmuGB::OpCB2C() { SRA(GetByteRegister<ByteRegisters::H_REGISTER>()); }
void EmuGB::OpCB2D() { SRA(GetByteRegister<ByteRegisters::L_REGISTER>()); }
void EmuGB::OpCB2E() {
	ui8 data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	SRA(data);
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), data);
	Cost<8>();
}
void EmuGB::OpCB2F() { SRA(GetByteRegister<ByteRegisters::A_REGISTER>()); }

void EmuGB::OpCB30() { Swap(GetByteRegister<ByteRegisters::B_REGISTER>()); }
void EmuGB::OpCB31() { Swap(GetByteRegister<ByteRegisters::C_REGISTER>()); }
void EmuGB::OpCB32() { Swap(GetByteRegister<ByteRegisters::D_REGISTER>()); }
void EmuGB::OpCB33() { Swap(GetByteRegister<ByteRegisters::E_REGISTER>()); }
void EmuGB::OpCB34() { Swap(GetByteRegister<ByteRegisters::H_REGISTER>()); }
void EmuGB::OpCB35() { Swap(GetByteRegister<ByteRegisters::L_REGISTER>()); }
void EmuGB::OpCB36() { 
	ui8 data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	Swap(data); 
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), data);
	Cost<8>();
}
void EmuGB::OpCB37() { Swap(GetByteRegister<ByteRegisters::A_REGISTER>()); }
void EmuGB::OpCB38() { SRL(GetByteRegister<ByteRegisters::B_REGISTER>()); }
void EmuGB::OpCB39() { SRL(GetByteRegister<ByteRegisters::C_REGISTER>()); }
void EmuGB::OpCB3A() { SRL(GetByteRegister<ByteRegisters::D_REGISTER>()); }
void EmuGB::OpCB3B() { SRL(GetByteRegister<ByteRegisters::E_REGISTER>()); }
void EmuGB::OpCB3C() { SRL(GetByteRegister<ByteRegisters::H_REGISTER>()); }
void EmuGB::OpCB3D() { SRL(GetByteRegister<ByteRegisters::L_REGISTER>()); }
void EmuGB::OpCB3E() {
	SRL(ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>()));
	Cost<8>();
}
void EmuGB::OpCB3F() { SRL(GetByteRegister<ByteRegisters::A_REGISTER>()); }

void EmuGB::OpCB40() { Bit(GetByteRegister<ByteRegisters::B_REGISTER>(), 0); }
void EmuGB::OpCB41() { Bit(GetByteRegister<ByteRegisters::C_REGISTER>(), 0); }
void EmuGB::OpCB42() { Bit(GetByteRegister<ByteRegisters::D_REGISTER>(), 0); }
void EmuGB::OpCB43() { Bit(GetByteRegister<ByteRegisters::E_REGISTER>(), 0); }
void EmuGB::OpCB44() { Bit(GetByteRegister<ByteRegisters::H_REGISTER>(), 0); }
void EmuGB::OpCB45() { Bit(GetByteRegister<ByteRegisters::L_REGISTER>(), 0); }
void EmuGB::OpCB46() {
	ui8 data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	Bit(data, 0);
	Cost<4>();
}
void EmuGB::OpCB47() { Bit(GetByteRegister<ByteRegisters::A_REGISTER>(), 0); }
void EmuGB::OpCB48() { Bit(GetByteRegister<ByteRegisters::B_REGISTER>(), 1); }
void EmuGB::OpCB49() { Bit(GetByteRegister<ByteRegisters::C_REGISTER>(), 1); }
void EmuGB::OpCB4A() { Bit(GetByteRegister<ByteRegisters::D_REGISTER>(), 1); }
void EmuGB::OpCB4B() { Bit(GetByteRegister<ByteRegisters::E_REGISTER>(), 1); }
void EmuGB::OpCB4C() { Bit(GetByteRegister<ByteRegisters::H_REGISTER>(), 1); }
void EmuGB::OpCB4D() { Bit(GetByteRegister<ByteRegisters::L_REGISTER>(), 1); }
void EmuGB::OpCB4E() {
	ui8 data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	Bit(data, 1);
	Cost<4>();
}
void EmuGB::OpCB4F() { Bit(GetByteRegister<ByteRegisters::A_REGISTER>(), 1); }

void EmuGB::OpCB50() { Bit(GetByteRegister<ByteRegisters::B_REGISTER>(), 2); }
void EmuGB::OpCB51() { Bit(GetByteRegister<ByteRegisters::C_REGISTER>(), 2); }
void EmuGB::OpCB52() { Bit(GetByteRegister<ByteRegisters::D_REGISTER>(), 2); }
void EmuGB::OpCB53() { Bit(GetByteRegister<ByteRegisters::E_REGISTER>(), 2); }
void EmuGB::OpCB54() { Bit(GetByteRegister<ByteRegisters::H_REGISTER>(), 2); }
void EmuGB::OpCB55() { Bit(GetByteRegister<ByteRegisters::L_REGISTER>(), 2); }
void EmuGB::OpCB56() {
	ui8 data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	Bit(data, 2);
	Cost<4>();
}
void EmuGB::OpCB57() { Bit(GetByteRegister<ByteRegisters::A_REGISTER>(), 2); }
void EmuGB::OpCB58() { Bit(GetByteRegister<ByteRegisters::B_REGISTER>(), 3); }
void EmuGB::OpCB59() { Bit(GetByteRegister<ByteRegisters::C_REGISTER>(), 3); }
void EmuGB::OpCB5A() { Bit(GetByteRegister<ByteRegisters::D_REGISTER>(), 3); }
void EmuGB::OpCB5B() { Bit(GetByteRegister<ByteRegisters::E_REGISTER>(), 3); }
void EmuGB::OpCB5C() { Bit(GetByteRegister<ByteRegisters::H_REGISTER>(), 3); }
void EmuGB::OpCB5D() { Bit(GetByteRegister<ByteRegisters::L_REGISTER>(), 3); }
void EmuGB::OpCB5E() {
	ui8 data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	Bit(data, 3);
	Cost<4>();
}
void EmuGB::OpCB5F() { Bit(GetByteRegister<ByteRegisters::A_REGISTER>(), 3); }

void EmuGB::OpCB60() { Bit(GetByteRegister<ByteRegisters::B_REGISTER>(), 4); }
void EmuGB::OpCB61() { Bit(GetByteRegister<ByteRegisters::C_REGISTER>(), 4); }
void EmuGB::OpCB62() { Bit(GetByteRegister<ByteRegisters::D_REGISTER>(), 4); }
void EmuGB::OpCB63() { Bit(GetByteRegister<ByteRegisters::E_REGISTER>(), 4); }
void EmuGB::OpCB64() { Bit(GetByteRegister<ByteRegisters::H_REGISTER>(), 4); }
void EmuGB::OpCB65() { Bit(GetByteRegister<ByteRegisters::L_REGISTER>(), 4); }
void EmuGB::OpCB66() {
	ui8 data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	Bit(data, 4);
	Cost<4>();
}
void EmuGB::OpCB67() { Bit(GetByteRegister<ByteRegisters::A_REGISTER>(), 4); }
void EmuGB::OpCB68() { Bit(GetByteRegister<ByteRegisters::B_REGISTER>(), 5); }
void EmuGB::OpCB69() { Bit(GetByteRegister<ByteRegisters::C_REGISTER>(), 5); }
void EmuGB::OpCB6A() { Bit(GetByteRegister<ByteRegisters::D_REGISTER>(), 5); }
void EmuGB::OpCB6B() { Bit(GetByteRegister<ByteRegisters::E_REGISTER>(), 5); }
void EmuGB::OpCB6C() { Bit(GetByteRegister<ByteRegisters::H_REGISTER>(), 5); }
void EmuGB::OpCB6D() { Bit(GetByteRegister<ByteRegisters::L_REGISTER>(), 5); }
void EmuGB::OpCB6E() {
	ui8 data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	Bit(data, 5);
	Cost<4>();
}
void EmuGB::OpCB6F() { Bit(GetByteRegister<ByteRegisters::A_REGISTER>(), 5); }

void EmuGB::OpCB70() { Bit(GetByteRegister<ByteRegisters::B_REGISTER>(), 6); }
void EmuGB::OpCB71() { Bit(GetByteRegister<ByteRegisters::C_REGISTER>(), 6); }
void EmuGB::OpCB72() { Bit(GetByteRegister<ByteRegisters::D_REGISTER>(), 6); }
void EmuGB::OpCB73() { Bit(GetByteRegister<ByteRegisters::E_REGISTER>(), 6); }
void EmuGB::OpCB74() { Bit(GetByteRegister<ByteRegisters::H_REGISTER>(), 6); }
void EmuGB::OpCB75() { Bit(GetByteRegister<ByteRegisters::L_REGISTER>(), 6); }
void EmuGB::OpCB76() {
	ui8 data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	Bit(data, 6);
	Cost<4>();
}
void EmuGB::OpCB77() { Bit(GetByteRegister<ByteRegisters::A_REGISTER>(), 6); }
void EmuGB::OpCB78() { Bit(GetByteRegister<ByteRegisters::B_REGISTER>(), 7); }
void EmuGB::OpCB79() { Bit(GetByteRegister<ByteRegisters::C_REGISTER>(), 7); }
void EmuGB::OpCB7A() { Bit(GetByteRegister<ByteRegisters::D_REGISTER>(), 7); }
void EmuGB::OpCB7B() { Bit(GetByteRegister<ByteRegisters::E_REGISTER>(), 7); }
void EmuGB::OpCB7C() { Bit(GetByteRegister<ByteRegisters::H_REGISTER>(), 7); }
void EmuGB::OpCB7D() { Bit(GetByteRegister<ByteRegisters::L_REGISTER>(), 7); }
void EmuGB::OpCB7E() {
	ui8 data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	Bit(data, 7);
	Cost<4>();
}
void EmuGB::OpCB7F() { Bit(GetByteRegister<ByteRegisters::A_REGISTER>(), 7); }

void EmuGB::OpCB80() { ClearBit(GetByteRegister<ByteRegisters::B_REGISTER>(), 0); }
void EmuGB::OpCB81() { ClearBit(GetByteRegister<ByteRegisters::C_REGISTER>(), 0); }
void EmuGB::OpCB82() { ClearBit(GetByteRegister<ByteRegisters::D_REGISTER>(), 0); }
void EmuGB::OpCB83() { ClearBit(GetByteRegister<ByteRegisters::E_REGISTER>(), 0); }
void EmuGB::OpCB84() { ClearBit(GetByteRegister<ByteRegisters::H_REGISTER>(), 0); }
void EmuGB::OpCB85() { ClearBit(GetByteRegister<ByteRegisters::L_REGISTER>(), 0); }
void EmuGB::OpCB86() {
	ui8& data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	ClearBit(data, 0);
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), data);
	Cost<8>();
}
void EmuGB::OpCB87() { ClearBit(GetByteRegister<ByteRegisters::A_REGISTER>(), 0); }
void EmuGB::OpCB88() { ClearBit(GetByteRegister<ByteRegisters::B_REGISTER>(), 1); }
void EmuGB::OpCB89() { ClearBit(GetByteRegister<ByteRegisters::C_REGISTER>(), 1); }
void EmuGB::OpCB8A() { ClearBit(GetByteRegister<ByteRegisters::D_REGISTER>(), 1); }
void EmuGB::OpCB8B() { ClearBit(GetByteRegister<ByteRegisters::E_REGISTER>(), 1); }
void EmuGB::OpCB8C() { ClearBit(GetByteRegister<ByteRegisters::H_REGISTER>(), 1); }
void EmuGB::OpCB8D() { ClearBit(GetByteRegister<ByteRegisters::L_REGISTER>(), 1); }
void EmuGB::OpCB8E() {
	ui8& data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	ClearBit(data, 1);
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), data);
	Cost<8>();
}
void EmuGB::OpCB8F() { ClearBit(GetByteRegister<ByteRegisters::A_REGISTER>(), 1); }

void EmuGB::OpCB90() { ClearBit(GetByteRegister<ByteRegisters::B_REGISTER>(), 2); }
void EmuGB::OpCB91() { ClearBit(GetByteRegister<ByteRegisters::C_REGISTER>(), 2); }
void EmuGB::OpCB92() { ClearBit(GetByteRegister<ByteRegisters::D_REGISTER>(), 2); }
void EmuGB::OpCB93() { ClearBit(GetByteRegister<ByteRegisters::E_REGISTER>(), 2); }
void EmuGB::OpCB94() { ClearBit(GetByteRegister<ByteRegisters::H_REGISTER>(), 2); }
void EmuGB::OpCB95() { ClearBit(GetByteRegister<ByteRegisters::L_REGISTER>(), 2); }
void EmuGB::OpCB96() {
	ui8& data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	ClearBit(data, 2);
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), data);
	Cost<8>();
}
void EmuGB::OpCB97() { ClearBit(GetByteRegister<ByteRegisters::A_REGISTER>(), 2); }
void EmuGB::OpCB98() { ClearBit(GetByteRegister<ByteRegisters::B_REGISTER>(), 3); }
void EmuGB::OpCB99() { ClearBit(GetByteRegister<ByteRegisters::C_REGISTER>(), 3); }
void EmuGB::OpCB9A() { ClearBit(GetByteRegister<ByteRegisters::D_REGISTER>(), 3); }
void EmuGB::OpCB9B() { ClearBit(GetByteRegister<ByteRegisters::E_REGISTER>(), 3); }
void EmuGB::OpCB9C() { ClearBit(GetByteRegister<ByteRegisters::H_REGISTER>(), 3); }
void EmuGB::OpCB9D() { ClearBit(GetByteRegister<ByteRegisters::L_REGISTER>(), 3); }
void EmuGB::OpCB9E() {
	ui8& data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	ClearBit(data, 3);
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), data);
	Cost<8>();
}
void EmuGB::OpCB9F() { ClearBit(GetByteRegister<ByteRegisters::A_REGISTER>(), 3); }

void EmuGB::OpCBA0() { ClearBit(GetByteRegister<ByteRegisters::B_REGISTER>(), 4); }
void EmuGB::OpCBA1() { ClearBit(GetByteRegister<ByteRegisters::C_REGISTER>(), 4); }
void EmuGB::OpCBA2() { ClearBit(GetByteRegister<ByteRegisters::D_REGISTER>(), 4); }
void EmuGB::OpCBA3() { ClearBit(GetByteRegister<ByteRegisters::E_REGISTER>(), 4); }
void EmuGB::OpCBA4() { ClearBit(GetByteRegister<ByteRegisters::H_REGISTER>(), 4); }
void EmuGB::OpCBA5() { ClearBit(GetByteRegister<ByteRegisters::L_REGISTER>(), 4); }
void EmuGB::OpCBA6() {
	ui8& data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	ClearBit(data, 4);
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), data);
	Cost<8>();
}
void EmuGB::OpCBA7() { ClearBit(GetByteRegister<ByteRegisters::A_REGISTER>(), 4); }
void EmuGB::OpCBA8() { ClearBit(GetByteRegister<ByteRegisters::B_REGISTER>(), 5); }
void EmuGB::OpCBA9() { ClearBit(GetByteRegister<ByteRegisters::C_REGISTER>(), 5); }
void EmuGB::OpCBAA() { ClearBit(GetByteRegister<ByteRegisters::D_REGISTER>(), 5); }
void EmuGB::OpCBAB() { ClearBit(GetByteRegister<ByteRegisters::E_REGISTER>(), 5); }
void EmuGB::OpCBAC() { ClearBit(GetByteRegister<ByteRegisters::H_REGISTER>(), 5); }
void EmuGB::OpCBAD() { ClearBit(GetByteRegister<ByteRegisters::L_REGISTER>(), 5); }
void EmuGB::OpCBAE() {
	ui8& data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	ClearBit(data, 5);
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), data);
	Cost<8>();
}
void EmuGB::OpCBAF() { ClearBit(GetByteRegister<ByteRegisters::A_REGISTER>(), 5); }

void EmuGB::OpCBB0() { ClearBit(GetByteRegister<ByteRegisters::B_REGISTER>(), 6); }
void EmuGB::OpCBB1() { ClearBit(GetByteRegister<ByteRegisters::C_REGISTER>(), 6); }
void EmuGB::OpCBB2() { ClearBit(GetByteRegister<ByteRegisters::D_REGISTER>(), 6); }
void EmuGB::OpCBB3() { ClearBit(GetByteRegister<ByteRegisters::E_REGISTER>(), 6); }
void EmuGB::OpCBB4() { ClearBit(GetByteRegister<ByteRegisters::H_REGISTER>(), 6); }
void EmuGB::OpCBB5() { ClearBit(GetByteRegister<ByteRegisters::L_REGISTER>(), 6); }
void EmuGB::OpCBB6() {
	ui8& data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	ClearBit(data, 6);
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), data);
	Cost<8>();
}
void EmuGB::OpCBB7() { ClearBit(GetByteRegister<ByteRegisters::A_REGISTER>(), 6); }
void EmuGB::OpCBB8() { ClearBit(GetByteRegister<ByteRegisters::B_REGISTER>(), 7); }
void EmuGB::OpCBB9() { ClearBit(GetByteRegister<ByteRegisters::C_REGISTER>(), 7); }
void EmuGB::OpCBBA() { ClearBit(GetByteRegister<ByteRegisters::D_REGISTER>(), 7); }
void EmuGB::OpCBBB() { ClearBit(GetByteRegister<ByteRegisters::E_REGISTER>(), 7); }
void EmuGB::OpCBBC() { ClearBit(GetByteRegister<ByteRegisters::H_REGISTER>(), 7); }
void EmuGB::OpCBBD() { ClearBit(GetByteRegister<ByteRegisters::L_REGISTER>(), 7); }
void EmuGB::OpCBBE() {
	ui8& data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	ClearBit(data, 7);
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), data);
	Cost<8>();
}
void EmuGB::OpCBBF() { ClearBit(GetByteRegister<ByteRegisters::A_REGISTER>(), 7); }

void EmuGB::OpCBC0() { SetBit(GetByteRegister<ByteRegisters::B_REGISTER>(), 0); }
void EmuGB::OpCBC1() { SetBit(GetByteRegister<ByteRegisters::C_REGISTER>(), 0); }
void EmuGB::OpCBC2() { SetBit(GetByteRegister<ByteRegisters::D_REGISTER>(), 0); }
void EmuGB::OpCBC3() { SetBit(GetByteRegister<ByteRegisters::E_REGISTER>(), 0); }
void EmuGB::OpCBC4() { SetBit(GetByteRegister<ByteRegisters::H_REGISTER>(), 0); }
void EmuGB::OpCBC5() { SetBit(GetByteRegister<ByteRegisters::L_REGISTER>(), 0); }
void EmuGB::OpCBC6() {
	ui8& data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	SetBit(data, 0);
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), data);
	Cost<8>();
}
void EmuGB::OpCBC7() { SetBit(GetByteRegister<ByteRegisters::A_REGISTER>(), 0); }
void EmuGB::OpCBC8() { SetBit(GetByteRegister<ByteRegisters::B_REGISTER>(), 1); }
void EmuGB::OpCBC9() { SetBit(GetByteRegister<ByteRegisters::C_REGISTER>(), 1); }
void EmuGB::OpCBCA() { SetBit(GetByteRegister<ByteRegisters::D_REGISTER>(), 1); }
void EmuGB::OpCBCB() { SetBit(GetByteRegister<ByteRegisters::E_REGISTER>(), 1); }
void EmuGB::OpCBCC() { SetBit(GetByteRegister<ByteRegisters::H_REGISTER>(), 1); }
void EmuGB::OpCBCD() { SetBit(GetByteRegister<ByteRegisters::L_REGISTER>(), 1); }
void EmuGB::OpCBCE() {
	ui8& data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	SetBit(data, 1);
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), data);
	Cost<8>();
}
void EmuGB::OpCBCF() { SetBit(GetByteRegister<ByteRegisters::A_REGISTER>(), 1); }

void EmuGB::OpCBD0() { SetBit(GetByteRegister<ByteRegisters::B_REGISTER>(), 2); }
void EmuGB::OpCBD1() { SetBit(GetByteRegister<ByteRegisters::C_REGISTER>(), 2); }
void EmuGB::OpCBD2() { SetBit(GetByteRegister<ByteRegisters::D_REGISTER>(), 2); }
void EmuGB::OpCBD3() { SetBit(GetByteRegister<ByteRegisters::E_REGISTER>(), 2); }
void EmuGB::OpCBD4() { SetBit(GetByteRegister<ByteRegisters::H_REGISTER>(), 2); }
void EmuGB::OpCBD5() { SetBit(GetByteRegister<ByteRegisters::L_REGISTER>(), 2); }
void EmuGB::OpCBD6() {
	ui8& data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	SetBit(data, 2);
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), data);
	Cost<8>();
}
void EmuGB::OpCBD7() { SetBit(GetByteRegister<ByteRegisters::A_REGISTER>(), 2); }
void EmuGB::OpCBD8() { SetBit(GetByteRegister<ByteRegisters::B_REGISTER>(), 3); }
void EmuGB::OpCBD9() { SetBit(GetByteRegister<ByteRegisters::C_REGISTER>(), 3); }
void EmuGB::OpCBDA() { SetBit(GetByteRegister<ByteRegisters::D_REGISTER>(), 3); }
void EmuGB::OpCBDB() { SetBit(GetByteRegister<ByteRegisters::E_REGISTER>(), 3); }
void EmuGB::OpCBDC() { SetBit(GetByteRegister<ByteRegisters::H_REGISTER>(), 3); }
void EmuGB::OpCBDD() { SetBit(GetByteRegister<ByteRegisters::L_REGISTER>(), 3); }
void EmuGB::OpCBDE() {
	ui8& data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	SetBit(data, 3);
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), data);
	Cost<8>();
}
void EmuGB::OpCBDF() { SetBit(GetByteRegister<ByteRegisters::A_REGISTER>(), 3); }

void EmuGB::OpCBE0() { SetBit(GetByteRegister<ByteRegisters::B_REGISTER>(), 4); }
void EmuGB::OpCBE1() { SetBit(GetByteRegister<ByteRegisters::C_REGISTER>(), 4); }
void EmuGB::OpCBE2() { SetBit(GetByteRegister<ByteRegisters::D_REGISTER>(), 4); }
void EmuGB::OpCBE3() { SetBit(GetByteRegister<ByteRegisters::E_REGISTER>(), 4); }
void EmuGB::OpCBE4() { SetBit(GetByteRegister<ByteRegisters::H_REGISTER>(), 4); }
void EmuGB::OpCBE5() { SetBit(GetByteRegister<ByteRegisters::L_REGISTER>(), 4); }
void EmuGB::OpCBE6() {
	ui8& data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	SetBit(data, 4);
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), data);
	Cost<8>();
}
void EmuGB::OpCBE7() { SetBit(GetByteRegister<ByteRegisters::A_REGISTER>(), 4); }
void EmuGB::OpCBE8() { SetBit(GetByteRegister<ByteRegisters::B_REGISTER>(), 5); }
void EmuGB::OpCBE9() { SetBit(GetByteRegister<ByteRegisters::C_REGISTER>(), 5); }
void EmuGB::OpCBEA() { SetBit(GetByteRegister<ByteRegisters::D_REGISTER>(), 5); }
void EmuGB::OpCBEB() { SetBit(GetByteRegister<ByteRegisters::E_REGISTER>(), 5); }
void EmuGB::OpCBEC() { SetBit(GetByteRegister<ByteRegisters::H_REGISTER>(), 5); }
void EmuGB::OpCBED() { SetBit(GetByteRegister<ByteRegisters::L_REGISTER>(), 5); }
void EmuGB::OpCBEE() {
	ui8& data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	SetBit(data, 5);
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), data);
	Cost<8>();
}
void EmuGB::OpCBEF() { SetBit(GetByteRegister<ByteRegisters::A_REGISTER>(), 5); }

void EmuGB::OpCBF0() { SetBit(GetByteRegister<ByteRegisters::B_REGISTER>(), 6); }
void EmuGB::OpCBF1() { SetBit(GetByteRegister<ByteRegisters::C_REGISTER>(), 6); }
void EmuGB::OpCBF2() { SetBit(GetByteRegister<ByteRegisters::D_REGISTER>(), 6); }
void EmuGB::OpCBF3() { SetBit(GetByteRegister<ByteRegisters::E_REGISTER>(), 6); }
void EmuGB::OpCBF4() { SetBit(GetByteRegister<ByteRegisters::H_REGISTER>(), 6); }
void EmuGB::OpCBF5() { SetBit(GetByteRegister<ByteRegisters::L_REGISTER>(), 6); }
void EmuGB::OpCBF6() {
	ui8& data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	SetBit(data, 6);
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), data);
	Cost<8>();
}
void EmuGB::OpCBF7() { SetBit(GetByteRegister<ByteRegisters::A_REGISTER>(), 6); }
void EmuGB::OpCBF8() { SetBit(GetByteRegister<ByteRegisters::B_REGISTER>(), 7); }
void EmuGB::OpCBF9() { SetBit(GetByteRegister<ByteRegisters::C_REGISTER>(), 7); }
void EmuGB::OpCBFA() { SetBit(GetByteRegister<ByteRegisters::D_REGISTER>(), 7); }
void EmuGB::OpCBFB() { SetBit(GetByteRegister<ByteRegisters::E_REGISTER>(), 7); }
void EmuGB::OpCBFC() { SetBit(GetByteRegister<ByteRegisters::H_REGISTER>(), 7); }
void EmuGB::OpCBFD() { SetBit(GetByteRegister<ByteRegisters::L_REGISTER>(), 7); }
void EmuGB::OpCBFE() {
	ui8& data = ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>());
	SetBit(data, 7);
	ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), data);
	Cost<8>();
}
void EmuGB::OpCBFF() { SetBit(GetByteRegister<ByteRegisters::A_REGISTER>(), 7); }