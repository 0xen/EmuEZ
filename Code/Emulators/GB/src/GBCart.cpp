#include <GBCart.hpp>
#include <IO.hpp>

#include <time.h>
#include <algorithm>
#include <assert.h>

EmuGBCart::EmuGBCart()
{
	mp_cart_data = nullptr;
	//LoadMemoryRule();
}

EmuGBCart::~EmuGBCart()
{
	if (mp_cart_data)
		delete[] mp_cart_data;
}

bool EmuGBCart::Load(const char* path, ui8* bus)
{
	mp_cart_data = LoadBinary(path, m_cart_size);
	if (!mp_cart_data)return false;

	m_bus = bus;

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
	m_name[10] = '\0';

	m_cb = mp_cart_data[PLATFORM] == 0x80;

	// What type of cartridge are we loading
	m_cartridge_type = static_cast<GBCartridgeType>(mp_cart_data[CARTRIDGE_TYPE]);

	LoadMemoryRule();

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

	m_rom_size_actual = std::max(Power2Celi(m_cart_size / 0x4000), 2u);
	

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

	// Bug in some games, its rom type says it has ram, but ram size was not reported, so we must give them max just incase
	if (m_memory_rule->HasRam() && m_ram_size == 0)
	{
		m_ram_size_bytes = 0x1024 * 128;
	}

	// If we need ram, load it
	if (m_ram_size > 0)
	{
		m_ram = new ui8[m_ram_size_bytes]{ 0 };
	}


	return true;
}

void EmuGBCart::Update()
{
	time(&m_currentTime);
}

bool EmuGBCart::IsCB()
{
	return m_cb;
}

void EmuGBCart::Reset()
{
	memcpy(m_bus, GetMBCRule()->GetBank0(), 0x4000);
	memcpy(&m_bus[0x4000], GetMBCRule()->GetCurrentBank1(), 0x4000);

	memset(&m_bus[0xA000], 0xFF, 0x2000);
}


ui8 EmuGBCart::RamBankCount()
{
	switch (m_ram_size)
	{
	case 0:
		return 0;
	case 1:
	case 2:
		return 1;
	case 3:
		return 4;
	case 4:
		return 16;
	}
	return 0;
}

unsigned int EmuGBCart::RomBankCount()
{
	return m_rom_size_actual;
}

time_t EmuGBCart::GetCurrentTime()
{
	return m_currentTime;
}

void EmuGBCart::LoadMemoryRule()
{
	switch (m_cartridge_type)
	{
	case GBCartridgeType::ROM_ONLY:
	{
		m_memory_rule = std::make_unique<MBCN<CartMBC::None, CartRam::None, CartBatt::None>>(this, m_bus);
	}
	break;
	case GBCartridgeType::ROM_AND_MBC1:
	{
		m_memory_rule = std::make_unique<MBCN<CartMBC::MBC1, CartRam::None, CartBatt::None>>(this, m_bus);
	}
	break;

	case GBCartridgeType::ROM_AND_MBC1_AND_RAM:
	{
		m_memory_rule = std::make_unique<MBCN<CartMBC::MBC1, CartRam::Avaliable, CartBatt::None>>(this, m_bus);
	}
	break;


	case GBCartridgeType::ROM_AND_MBC1_AND_RAM_AND_BATT:
	{
		m_memory_rule = std::make_unique<MBCN<CartMBC::MBC1, CartRam::Avaliable, CartBatt::Avaliable>>(this, m_bus);
	}
	break;

	case GBCartridgeType::ROM_ANDMMMD1_AND_SRAM_AND_BATT:
	{
		m_memory_rule = std::make_unique<MBCN<CartMBC::MBC3, CartRam::Avaliable, CartBatt::Avaliable>>(this, m_bus);
	}
	break;

	case GBCartridgeType::ROM_AND_MBC3_AND_RAM_BATT:
	{
		m_memory_rule = std::make_unique<MBCN<CartMBC::MBC3, CartRam::Avaliable, CartBatt::Avaliable>>(this, m_bus);
	}
	break;

	case GBCartridgeType::ROM_AND_MBC5_AND_RAM_AND_BATT:
	{
		m_memory_rule = std::make_unique<MBCN<CartMBC::MBC5, CartRam::Avaliable, CartBatt::Avaliable>>(this, m_bus);
	}
	break;

	default:
		assert(0&&"Memory Rule not recognised");
		break;
	}
}

unsigned int EmuGBCart::Power2Celi(unsigned int data)
{
	--data;
	data |= data >> 1;
	data |= data >> 2;
	data |= data >> 4;
	data |= data >> 8;
	++data;
	return data;
}
