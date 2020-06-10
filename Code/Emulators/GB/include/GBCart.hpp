#pragma once
#include "../../../Util/include/Definitions.hpp"

#include <type_traits>

enum GBCartridgeType
{
	ROM_ONLY = 0x00,
	ROM_AND_MBC1 = 0x01,
	ROM_AND_MBC1_AND_RAM = 0x02,
	ROM_AND_MBC1_AND_RAM_AND_BATT = 0x03,
	ROM_AND_MBC2 = 0x05,
	ROM_AND_MBC2_AND_BATT = 0x06,
	ROM_AND_RAM = 0x08,
	ROM_AND_RAM_AND_BATT = 0x09,
	ROM_AND_MMMD1 = 0x0B,
	ROM_AND_MMMD1_AND_SRAM = 0x0C,
	ROM_AND_MMMD1_AND_SRAM_AND_BATT = 0x0D,
	ROM_AND_MBC3_AND_TIMER_AND_BATT = 0x0F,
	ROM_ANDMMMD1_AND_SRAM_AND_BATT = 0x10,
	ROM_AND_MBC3 = 0x11,
	ROM_AND_MBC3_AND_RAM = 0x12,
	ROM_AND_MBC3_AND_RAM_BATT = 0x13,
	ROM_AND_MBC5 = 0x19,
	ROM_AND_MBC5_AND_RAM = 0x1A,
	ROM_AND_MBC5_AND_RAM_AND_BATT = 0x1B,
	ROM_AND_MBC5_AND_RUMBLE = 0x1C,
	ROM_AND_MBC5_AND_RUMBLE_AND_SRAM = 0x1D,
	ROM_AND_MBC5_AND_RUMBLE_AND_SRAM_AND_BATT = 0x1E,
	POCKET_CAMERA = 0x1F,
	BANDAI_TAMA5 = 0xFD,
	HUDSON_HUC_3 = 0xFE,
	HUDSON_HUC_1 = 0xFF
};

/*
SRC : http://gbdev.gg8.se/wiki/articles/The_Cartridge_Header
0x100 -> 0x103 : Entry Point
0x104 -> 0x133 : Nintendo logo
0x134 -> 0x143 : Title Start (10 bytes long) Uppercase Ascii
0x143 : Platform (CB or not cb...dats the qestion)
0x144 -> 0x145 : Licence code (Will be 0x0000 as we do not have a licence))
0x146 : Dose the game support SGB functions
0x147 : Cartridge Type
0x148 : Rom Size
0x149 : Ram Size
0x14A : Distination Code
0x14B : Old Licence code
0x14C : Game Version (Normally 0x00)
0x14D : Header checksum (x=0:FOR i=0134h TO 014Ch:x=x-MEM[i]-1:NEXT)
0x14E -> 0x14F : Global Checksum
*/

// Memory lookup codes
static const unsigned int TITLE_MEMORY_LOCATION = 0x0134;
static const unsigned int DISTINATION_CODE = 0x014A;
static const unsigned int PLATFORM = 0x0143;
static const unsigned int CARTRIDGE_TYPE = 0x0147;
static const unsigned int ROM_SIZE = 0x0148;
static const unsigned int RAM_SIZE = 0x0149;

struct EmuGBCart
{
public:
	EmuGBCart();
	~EmuGBCart();
	bool Load(const char* path);

	template<MemoryAccessType type, class D, class T>
	void ProcessROM(ui16 address, std::conditional_t<type == MemoryAccessType::Read, T&, T> data)
	{
		ProcessMemory<type>(mp_cart_data, address, data);
	}

	template<MemoryAccessType type, class D, class T>
	void ProcessRAM(ui16 address, std::conditional_t<type == MemoryAccessType::Read, T&, T> data)
	{

	}
private:
	ui8* mp_cart_data;
	unsigned int m_cart_size;
	ui8* m_ram = nullptr;
	unsigned int m_ram_size_bytes;

	char m_name[11];
	bool m_cb;
	unsigned int m_rom_size;
	unsigned int m_ram_size;
	unsigned int m_destination_code;
	GBCartridgeType m_cartridge_type;
};