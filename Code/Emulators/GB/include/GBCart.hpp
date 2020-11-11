#pragma once
#include <Definitions.hpp>

#include <cstring>
#include <type_traits>
#include <memory>
#include <fstream>

enum class GBCartridgeType
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

enum class CartMBC : ui32
{
	None,
	MBC1,
	MBC2,
	MBC3,
	MBC5
};

enum class CartRam : ui32
{
	None,
	Avaliable
	/*KB_2,
	KB_8,
	KB_32,
	KB_128,*/
};

enum class CartBatt : ui32
{
	None,
	Avaliable
};
struct RTC
{
	i32 Seconds;
	i32 Minutes;
	i32 Hours;
	i32 Days;
	i32 Control;
	i32 LatchedSeconds;
	i32 LatchedMinutes;
	i32 LatchedHours;
	i32 LatchedDays;
	i32 LatchedControl;
	i32 LastTime;
	i32 padding;
};

class EmuGBCart;
class MBCRule
{
protected:
	EmuGBCart* m_cart;
	ui8* m_bus;
public:
	MBCRule(EmuGBCart* cart, ui8* bus) : m_cart(cart), m_bus(bus)
	{

	}
	virtual ~MBCRule() {}

	virtual ui8* GetBank0() = 0;

	virtual ui8* GetCurrentBank1() = 0;

	virtual void Write(ui16 address, ui8 data) = 0;

	virtual bool HasRam() = 0;
};


template<CartMBC cartMBC = CartMBC::None, CartRam cartRam = CartRam::None, CartBatt cartBatt = CartBatt::None>
class MBCN : public MBCRule
{
	int m_memory_bank;
	ui8 m_rom_bank_high;
	ui8 m_mode;
	int m_ram_bank;
	ui16 m_ram_offset;
	bool m_ram_enabled;
	bool m_timer_enabled;
	i32 m_iRTCLatch;
	RTC m_RTC;
	ui8 m_RTC_reg;
public:
	MBCN(EmuGBCart* cart, ui8* bus);

	virtual ~MBCN();

	virtual ui8* GetBank0();

	virtual ui8* GetCurrentBank1();

	virtual void Write(ui16 address, ui8 data);

	virtual bool HasRam() {
		return cartRam != CartRam::None;
	};

private:

	void WriteToMBC1(ui16 address, ui8 data);

	void WriteToMBC2(ui16 address, ui8 data);

	void WriteToMBC3(ui16 address, ui8 data);

	void WriteToMBC5(ui16 address, ui8 data);
	
	void RamEnable(ui8 data);

	void RomBankChange(ui16 address, ui8 data);

	void RamBankChange(ui8 data);

	void RamAndRomChange(ui8 data);

	void RamBankWrite(ui16 address, ui8 data);

	void RamAndRTCChange(ui8 data);

	void LatchRTC(ui8 data);

	void UpdateRamBank();

	void UpdateRTC();
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

class EmuGBCart
{
	time_t m_currentTime;
public:
	EmuGBCart();
	~EmuGBCart();
	bool Load(const char* path, ui8* bus);

	void SaveRam(std::ostream& stream );

	void LoadRam(std::istream& stream );

	ui8* GetRawData()
	{
		return mp_cart_data;
	}

	void Update();

	bool IsCB();

	void Reset();

	unsigned int RamBankSize();

	ui8 RamBankCount();

	int RomBankCount();

	ui8* GetRawRamMemory() { return m_ram; }

	time_t GetCurrentTime();

	MBCRule* GetMBCRule() { return m_memory_rule.get(); }

private:

	void LoadMemoryRule();

	unsigned int Power2Celi(unsigned int data);

	std::unique_ptr<MBCRule> m_memory_rule;


	ui8* mp_cart_data;
	ui8* m_bus = nullptr;
	unsigned int m_cart_size;
	ui8* m_ram = nullptr;
	unsigned int m_ram_size_bytes;

	char m_name[11];
	bool m_cb;
	unsigned int m_rom_size;
	unsigned int m_rom_size_actual;
	unsigned int m_ram_size;
	unsigned int m_destination_code;
	GBCartridgeType m_cartridge_type;
};

template<CartMBC cartMBC, CartRam cartRam, CartBatt cartBatt>
inline MBCN<cartMBC, cartRam, cartBatt>::MBCN(EmuGBCart* cart, ui8* bus) : MBCRule(cart, bus)
{
	m_memory_bank = 1;
	m_mode = 0;
	m_rom_bank_high = 0;
	m_ram_bank = 0;
	m_ram_offset = 0;
	m_ram_enabled = false;
	m_timer_enabled = false;
	m_RTC = {};
	m_RTC_reg = 0;
	m_iRTCLatch = 0;
}

template<CartMBC cartMBC, CartRam cartRam, CartBatt cartBatt>
inline MBCN<cartMBC, cartRam, cartBatt>::~MBCN()
{
}

template<CartMBC cartMBC, CartRam cartRam, CartBatt cartBatt>
inline ui8* MBCN<cartMBC, cartRam, cartBatt>::GetBank0()
{
	return &m_cart->GetRawData()[0x0000];
}

template<CartMBC cartMBC, CartRam cartRam, CartBatt cartBatt>
inline ui8* MBCN<cartMBC, cartRam, cartBatt>::GetCurrentBank1()
{
	if constexpr (cartMBC == CartMBC::None)
	{
		return &m_cart->GetRawData()[0x4000];
	}
	else
	{
		return &m_cart->GetRawData()[0x4000 * m_memory_bank];
	}
}

template<CartMBC cartMBC, CartRam cartRam, CartBatt cartBatt>
inline void MBCN<cartMBC, cartRam, cartBatt>::Write(ui16 address, ui8 data)
{
	if constexpr (cartMBC == CartMBC::MBC1)
	{
		WriteToMBC1(address, data);
	}
	else if constexpr (cartMBC == CartMBC::MBC2)
	{
		WriteToMBC2(address, data);
	}
	else if constexpr (cartMBC == CartMBC::MBC3)
	{
		WriteToMBC3(address, data);
	}
	else if constexpr (cartMBC == CartMBC::MBC5)
	{
		WriteToMBC5(address, data);
	}
}

template<CartMBC cartMBC, CartRam cartRam, CartBatt cartBatt>
inline void MBCN<cartMBC, cartRam, cartBatt>::WriteToMBC1(ui16 address, ui8 data)
{

	switch (address & 0xE000)
	{
	case 0x0000: // Ram Enable
		RamEnable(data);
		return;
	case 0x2000:
		RomBankChange(address, data);
		return;
	case 0x4000:
		RamAndRomChange(data);
		return;
	case 0x6000: // Ram-Rom Mode
		if (!((m_cart->RamBankSize() != 3) && (data & 0x01)))
		{
			m_mode = data & 0x01;
			//if constexpr (cartRam == CartRam::Avaliable)
			//{
			//	if (m_mode == 0)
			//	{
			//		m_ram_bank = 0;
			//		m_ram_offset = (ui8)m_ram_bank * 0x2000;
			//		// Load memory bank 1
			//		UpdateRamBank();
			//	}
			//}
		}
		return;
	case 0xA000:
		RamBankWrite(address, data);
		return;
	}

}

template<CartMBC cartMBC, CartRam cartRam, CartBatt cartBatt>
inline void MBCN<cartMBC, cartRam, cartBatt>::WriteToMBC2(ui16 address, ui8 data)
{
}

template<CartMBC cartMBC, CartRam cartRam, CartBatt cartBatt>
inline void MBCN<cartMBC, cartRam, cartBatt>::WriteToMBC3(ui16 address, ui8 data)
{
	ui16 setting = address & 0xE000;
	switch (setting)
	{
	case 0x0000: // Ram Enable
		RamEnable(data);
		return;
	case 0x2000:
		RomBankChange(address, data);
		return;
	case 0x4000:
		RamAndRTCChange(data);
		return;
	case 0x6000:
		LatchRTC(data);
		return;
	case 0xA000:
		RamBankWrite(address, data);
		return;
	default:
		m_bus[address] = data;
	}
}

template<CartMBC cartMBC, CartRam cartRam, CartBatt cartBatt>
inline void MBCN<cartMBC, cartRam, cartBatt>::WriteToMBC5(ui16 address, ui8 data)
{
	ui16 setting = address & 0xF000;


	switch (setting)
	{
	case 0x0000: // Ram Enable
		RamEnable(data);
		return;
	case 0x2000:
		RomBankChange(address, data);
		return;
	case 0x4000:
		RamBankChange(data);
		return;
	case 0x6000:
		// Nope
		return;
	case 0xA000:
		RamBankWrite(address, data);
		return;
	}
}

template<CartMBC cartMBC, CartRam cartRam, CartBatt cartBatt>
inline void MBCN<cartMBC, cartRam, cartBatt>::RamEnable(ui8 data)
{
	if constexpr (cartMBC == CartMBC::MBC3)
	{
		m_timer_enabled = ((data & 0x0F) == 0x0A);
	}

	// Dose the cart support ram
	if constexpr (cartRam == CartRam::Avaliable)
	{
		if ( m_cart->RamBankSize() > 0 )
		{
			m_ram_enabled = ((data & 0x0F) == 0x0A);
			UpdateRamBank( );
		}

	}
	else
	{
		// Attempting to toggle RAM when RAM not supported
	}
}

template<CartMBC cartMBC, CartRam cartRam, CartBatt cartBatt>
inline void MBCN<cartMBC, cartRam, cartBatt>::RomBankChange(ui16 address, ui8 data)
{
	// Change rom bank
	if constexpr (cartMBC == CartMBC::MBC1)
	{
		if (m_mode == 0)
		{
			m_memory_bank = (data & 0x1F) | (m_rom_bank_high << 5);
		}
		else
		{
			m_memory_bank = data & 0x1f;
		}
		// If rom bank is set to 0x00,0x20,0x40,0x60 we incroment it

		if (m_memory_bank == 0x00 || m_memory_bank == 0x20 ||
			m_memory_bank == 0x40 || m_memory_bank == 0x60)
			m_memory_bank++;


		m_memory_bank &= (m_cart->RomBankCount() - 1);
	}
	else if constexpr (cartMBC == CartMBC::MBC3)
	{
		m_memory_bank = data & 0x7F;
		// Docs say that if the bank we are setting to is 0, we set to 1 insted
		if (m_memory_bank == 0) m_memory_bank = 1;

		m_memory_bank &= (m_cart->RomBankCount() - 1);
	}
	else if constexpr (cartMBC == CartMBC::MBC5)
	{
		if (address < 0x3000) // Low 8 bits of ROM bank
		{
			m_memory_bank = data | (m_rom_bank_high << 8);
		}
		else
		{
			m_rom_bank_high = data & 0x01;
			m_memory_bank = (m_memory_bank & 0xFF) | (m_rom_bank_high << 8);
		}
	}

	// Load into memory bank 1
	std::memcpy(&m_bus[0x4000], GetCurrentBank1(), 0x4000);
}

template<CartMBC cartMBC, CartRam cartRam, CartBatt cartBatt>
inline void MBCN<cartMBC, cartRam, cartBatt>::RamBankChange(ui8 data)
{
	m_ram_bank = data & 0x0F;
	m_ram_bank &= (m_cart->RamBankCount() - 1);
	m_ram_offset = (ui8)m_ram_bank * 0x2000;
	// Load memory bank 1
	UpdateRamBank();
}

template<CartMBC cartMBC, CartRam cartRam, CartBatt cartBatt>
inline void MBCN<cartMBC, cartRam, cartBatt>::RamAndRomChange(ui8 data)
{
	if (m_mode == 1)
	{
		if constexpr (cartRam == CartRam::Avaliable)
		{
			m_ram_bank = data & 0x03;
			m_ram_bank &= (m_cart->RamBankCount() - 1);
			m_ram_offset = (ui8)m_ram_bank * 0x2000;
			// Load memory bank 1
			UpdateRamBank();
		}
	}
	else
	{
		m_rom_bank_high = data & 0x03;
		m_memory_bank = (m_memory_bank & 0x1F) | (m_rom_bank_high << 5);
		// If rom bank is set to 0x00,0x20,0x40,0x60 we incroment it
		if (m_memory_bank == 0x00 || m_memory_bank == 0x20 ||
			m_memory_bank == 0x40 || m_memory_bank == 0x60)
			m_memory_bank++;

		m_memory_bank &= (m_cart->RomBankCount() - 1);

		// Load into memory bank 1
		std::memcpy(&m_bus[0x4000], GetCurrentBank1(), 0x4000);
	}
}

template<CartMBC cartMBC, CartRam cartRam, CartBatt cartBatt>
inline void MBCN<cartMBC, cartRam, cartBatt>::RamBankWrite(ui16 address, ui8 data)
{
	if constexpr (cartMBC == CartMBC::MBC1 || cartMBC == CartMBC::MBC5)
	{
		if (m_ram_enabled)
		{
			if constexpr (cartRam == CartRam::Avaliable)
			{

				if (m_mode == 0)
				{
					m_bus[address] = m_cart->GetRawRamMemory()[address - 0xA000] = data;
				}
				else
				{
					m_bus[address] = m_cart->GetRawRamMemory()[(address - 0xA000) + m_ram_offset] = data;
				}


			}
			else
			{
				// Attempting to write to RAM when RAM not supported
			}
		}
	}
	else if constexpr (cartMBC == CartMBC::MBC3)
	{
		if (m_ram_bank >= 0)
		{
			if (m_ram_enabled)
			{
				if constexpr (cartRam == CartRam::Avaliable)
				{
					m_bus[address] = m_cart->GetRawRamMemory()[(address - 0xA000) + m_ram_offset] = data;
				}
				else
				{
					// Attempting to write to RAM when RAM not supported
				}
			}
		}
		else if (m_timer_enabled)
		{
			switch (m_RTC_reg)
			{
			case 0x08:
				m_RTC.Seconds = data;
				break;
			case 0x09:
				m_RTC.Minutes = data;
				break;
			case 0x0A:
				m_RTC.Hours = data;
				break;
			case 0x0B:
				m_RTC.Days = data;
				break;
			case 0x0C:
				m_RTC.Control = (m_RTC.Control & 0x80) | (data & 0xC1);
				break;
			default:
				// Attempting to set RTC data when not supported
				break;
			}
		}
		else
		{
			// Attempting to write to RTC when RTC is disabled
		}
	}
}

template<CartMBC cartMBC, CartRam cartRam, CartBatt cartBatt>
inline void MBCN<cartMBC, cartRam, cartBatt>::RamAndRTCChange(ui8 data)
{
	if (data <= 0x03)
	{
		m_ram_bank = data;
		m_ram_bank &= (m_cart->RamBankCount() - 1);
		m_ram_offset = (ui8)m_ram_bank * 0x2000;
		// Load memory bank 1
		UpdateRamBank();
	}
	else if ((data >= 0x08) && (data <= 0x0C))
	{

		if (m_timer_enabled)
		{
			m_RTC_reg = data;
			m_ram_bank = -1;
		}
		else
		{
			// Attempting to use RTC when dissabled
		}

	}
	else
	{
		// Invalid
		// Attempting to write to invalid RAM / RTC register
	}
}

template<CartMBC cartMBC, CartRam cartRam, CartBatt cartBatt>
inline void MBCN<cartMBC, cartRam, cartBatt>::LatchRTC(ui8 data)
{
	// RTC Latch
	if ((m_iRTCLatch == 0x00) && (data == 0x01))
	{
		UpdateRTC();
		m_RTC.LatchedSeconds = m_RTC.Seconds;
		m_RTC.LatchedMinutes = m_RTC.Minutes;
		m_RTC.LatchedHours = m_RTC.Hours;
		m_RTC.LatchedDays = m_RTC.Days;
		m_RTC.LatchedControl = m_RTC.Control;
	}

	m_iRTCLatch = data;
}

template<CartMBC cartMBC, CartRam cartRam, CartBatt cartBatt>
inline void MBCN<cartMBC, cartRam, cartBatt>::UpdateRamBank()
{
	if (m_ram_enabled && m_ram_bank >= 0)
	{
		if (m_cart->RamBankCount() == 1)
		{
			// Only 2kb of ram is visible, so null out the other 2kb
			std::memcpy(&m_bus[0xA000], m_cart->GetRawRamMemory(), 0x1000);
			memset(&m_bus[0xB000], 0xFF, 0x1000);
		}
		else
		{
			std::memcpy(&m_bus[0xA000], &m_cart->GetRawRamMemory()[m_ram_offset], 0x2000);
		}
	}
	else
	{
		// Clear the ram as it is dissabled
		memset(&m_bus[0xA000], 0xFF, 0x2000);
	}
}

template<CartMBC cartMBC, CartRam cartRam, CartBatt cartBatt>
inline void MBCN<cartMBC, cartRam, cartBatt>::UpdateRTC()
{
	i32 currentTime = static_cast<i32>(m_cart->GetCurrentTime());
	
	if ((m_RTC.Control >> 6) & 1)
	{
		i32 difference = currentTime - m_RTC.LastTime;
		m_RTC.LastTime = currentTime;


		if (difference > 0)
		{
			// Secconds
			m_RTC.Seconds += (i32)(difference % 60);

			if (m_RTC.Seconds > 59)
			{
				m_RTC.Seconds -= 60;
				m_RTC.Minutes++;
			}

			difference /= 60;

			// Minuts
			m_RTC.Minutes += (i32)(difference % 60);

			if (m_RTC.Minutes > 59)
			{
				m_RTC.Minutes -= 60;
				m_RTC.Hours++;
			}

			difference /= 60;
			// Hours
			m_RTC.Hours += (i32)(difference % 24);

			if (m_RTC.Hours > 23)
			{
				m_RTC.Hours -= 24;
				m_RTC.Days++;
			}

			difference /= 24;
			// Days
			m_RTC.Days += (i32)(difference & 0xffffffff);

			if (m_RTC.Days > 0xFF)
			{
				m_RTC.Control = (m_RTC.Control & 0xC1) | 0x01;

				if (m_RTC.Days > 511)
				{
					m_RTC.Days %= 512;
					m_RTC.Control |= 0x80;
					m_RTC.Control &= 0xC0;
				}
			}
		}

	}
}
