#include "..\include\GBCart.hpp"
#include "../../../Util/include/IO.hpp"

EmuGBCart::EmuGBCart()
{
	mp_cart_data = nullptr;
}

EmuGBCart::~EmuGBCart()
{
	if (mp_cart_data)
		delete[] mp_cart_data;
}

bool EmuGBCart::Load(const char* path)
{
	mp_cart_data = LoadBinary(path, m_cart_size);
	if (!mp_cart_data)return false;


	// Load cartridge name
	for (int i = 0; i < 10; i++)
	{
		m_name[i] = mp_cart_data[TITLE_MEMORY_LOCATION + i];

		if (m_name[i] == 0)
		{
			m_name[i] = '\0';
			break;
		}
	}
	m_name[11] = '\0';

	// What type of cartridge are we loading
	m_cartridge_type = static_cast<GBCartridgeType>(mp_cart_data[CARTRIDGE_TYPE]);

	/*
	0: Japanese
	1: Non-Japanese
	*/
	m_destination_code = mp_cart_data[DISTINATION_CODE];



	// Rom size
	/*
	0   | 256Kbit | 32KB  | 2 banks
	1   | 512Kbit | 64KB  | 4 banks
	2   | 1Mbit   | 128KB | 8 banks
	3   | 2Mbit   | 256KB | 16 banks
	4   | 4Mbit   | 512KB | 32 banks
	5   | 8Mbit   | 1MB   | 64 banks
	6   | 16Mbit  | 2MB   | 128 banks
	$52 | 9Mbit   | 1.1MB | 72 banks
	$53 | 10Mbit  | 1.2MB | 80 banks
	$54 | 12Mbit  | 1.5MB | 96 banks
	*/
	m_rom_size = mp_cart_data[ROM_SIZE];

	/*
	0 | None
	1 | 16kBit  | 2kB   | 1 bank
	2 | 64kBit  | 8kB   | 1 bank
	3 | 256kBit | 32kB  | 4 banks
	4 | 1MBit   | 128kB |16 banks
	*/
	m_ram_size = mp_cart_data[RAM_SIZE];

	switch (m_ram_size)
	{
	case 1:
		m_ram_size_bytes = 0x1024 * 2;
		break;
	case 2:
		m_ram_size_bytes = 0x1024 * 8;
		break;
	case 3:
		m_ram_size_bytes = 0x1024 * 32;
		break;
	case 4:
		m_ram_size_bytes = 0x1024 * 128;
		break;
	}

	// If we need ram, load it
	if (m_ram_size > 0)
	{
		m_ram = new ui8[m_ram_size_bytes]{ 0 };
	}


	return true;
}
