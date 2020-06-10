#pragma once
#include <Base.hpp>
#include "..\include\GBCart.hpp"


/* From the Gearboy emulator */
const unsigned int bootDMGSize = 256;
const ui8 bootDMG[bootDMGSize] = {
	// Origional Nintendo
	0x31, 0xfe, 0xff, 0xaf, 0x21, 0xff, 0x9f, 0x32, 0xcb, 0x7c, 0x20, 0xfb, 0x21, 0x26, 0xff, 0x0e,
	0x11, 0x3e, 0x80, 0x32, 0xe2, 0x0c, 0x3e, 0xf3, 0xe2, 0x32, 0x3e, 0x77, 0x77, 0x3e, 0xfc, 0xe0,
	0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1a, 0xcd, 0x95, 0x00, 0xcd, 0x96, 0x00, 0x13, 0x7b,
	0xfe, 0x34, 0x20, 0xf3, 0x11, 0xd8, 0x00, 0x06, 0x08, 0x1a, 0x13, 0x22, 0x23, 0x05, 0x20, 0xf9,
	0x3e, 0x19, 0xea, 0x10, 0x99, 0x21, 0x2f, 0x99, 0x0e, 0x0c, 0x3d, 0x28, 0x08, 0x32, 0x0d, 0x20,
	0xf9, 0x2e, 0x0f, 0x18, 0xf3, 0x67, 0x3e, 0x64, 0x57, 0xe0, 0x42, 0x3e, 0x91, 0xe0, 0x40, 0x04,
	0x1e, 0x02, 0x0e, 0x0c, 0xf0, 0x44, 0xfe, 0x90, 0x20, 0xfa, 0x0d, 0x20, 0xf7, 0x1d, 0x20, 0xf2,
	0x0e, 0x13, 0x24, 0x7c, 0x1e, 0x83, 0xfe, 0x62, 0x28, 0x06, 0x1e, 0xc1, 0xfe, 0x64, 0x20, 0x06,
	0x7b, 0xe2, 0x0c, 0x3e, 0x87, 0xe2, 0xf0, 0x42, 0x90, 0xe0, 0x42, 0x15, 0x20, 0xd2, 0x05, 0x20,
	0x4f, 0x16, 0x20, 0x18, 0xcb, 0x4f, 0x06, 0x04, 0xc5, 0xcb, 0x11, 0x17, 0xc1, 0xcb, 0x11, 0x17,
	0x05, 0x20, 0xf5, 0x22, 0x23, 0x22, 0x23, 0xc9, 0xce, 0xed, 0x66, 0x66, 0xcc, 0x0d, 0x00, 0x0b,
	0x03, 0x73, 0x00, 0x83, 0x00, 0x0c, 0x00, 0x0d, 0x00, 0x08, 0x11, 0x1f, 0x88, 0x89, 0x00, 0x0e,
	0xdc, 0xcc, 0x6e, 0xe6, 0xdd, 0xdd, 0xd9, 0x99, 0xbb, 0xbb, 0x67, 0x63, 0x6e, 0x0e, 0xec, 0xcc,
	0xdd, 0xdc, 0x99, 0x9f, 0xbb, 0xb9, 0x33, 0x3e, 0x3c, 0x42, 0xb9, 0xa5, 0xb9, 0xa5, 0x42, 0x3c,
	0x21, 0x04, 0x01, 0x11, 0xa8, 0x00, 0x1a, 0x13, 0xbe, 0x20, 0xfe, 0x23, 0x7d, 0xfe, 0x34, 0x20,
	0xf5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xfb, 0x86, 0x20, 0xfe, 0x3e, 0x01, 0xe0, 0x50
};


struct EmuGB : public EmuBase<EmuGB>
{
	friend struct EmuBase<EmuGB>;
	EmuGB();
private:
	bool InitEmu(const char* path);

	void TickEmu();

	void InitOPJumpTables();

	void Reset();

	__forceinline bool InMemoryRange(ui16 start, ui16 end, ui16 address);

	template<MemoryAccessType type, class U, class V>
	void ProcessBus(U address, std::conditional_t<type == MemoryAccessType::Read, V&, V> data)
	{
		static constexpr int dataSize = sizeof(V);

		if constexpr (type == MemoryAccessType::Read)
		{
			data = ProcessBusReadRef<U,V>(address);
		}
		else if constexpr (type == MemoryAccessType::Write)
		{
			// Video Memory
			if (InMemoryRange(0x8000, 0x9FFF, address)) // High Frequancy
			{
				m_bus_memory[address] = data;
				return;
			}

			// Main Ram
			if (InMemoryRange(0xC000, 0xDFFF, address)) // High Frequancy
			{
				m_bus_memory[address] = data;
				return;
			}

			// Echo Ram
			if (InMemoryRange(0xE000, 0xFDFF, address)) // High Frequancy
			{
				// Since we are not supposed to read or write to this space, we shall access the main ram instead
				m_bus_memory[address - 0x2000] = data;
				return;
			}

			// I/O
			if (InMemoryRange(0xFF00, 0xFF7F, address)) // High Frequancy
			{
				switch (address)
				{
				case 0xFF00: // Input
				{
					m_bus_memory[address] = data;
					// To do: Joypad
					break;
				}
				case 0xFF04: // Timer divider
				{
					m_bus_memory[address] = 0x0;
					break;
				}
				case 0xFF07: // Timer Control
				{
					// To do: Timer Control
					break;
				}
				case 0xFF40: // Video Control
				{
					// To do: Video Control
					break;
				}
				case 0xFF44: // Video Line Val. Reset if wrote too
				{
					m_bus_memory[address] = 0;
					break;
				}
				case 0xFF46: // Transfer Sprites DMA
				{
					// To do: DMA transfer
					break;
				}
				case 0xFF50: // Boot rom switch
				{
					// To do, switch from BIOS to cart and back
					m_bus_memory[address] = data;
					break;
				}
				default:
				{
					// Video BG Palette
					// Video Sprite Palette 0
					// Video Sprite Palette 1
					// Video Window Y
					// Video Window X
					// Serial
					// Video LYC
					// Video Status
					// Video Scroll Y
					// Video Scroll X
					// CPU Interrupt Flag
					// Timer
					// Timer Modulo
					m_bus_memory[address] = data;
				}
				}
				return;
			}

			// Cart Ram
			if (InMemoryRange(0xA000, 0xBFFF, address)) // Med Frequancy
			{
				// To do: Cart Ram
				return;
			}

			// OAM - Object Attribute Memory
			if (InMemoryRange(0xFE00, 0xFE9F, address)) // Med Frequancy
			{
				m_bus_memory[address] = data;
				return;
			}

			// Cartridge ROM
			if (InMemoryRange(0x0000, 0x7FFF, address)) // Low Frequancy
			{
				// To do: MBC Rule change
				return;
			}

			// Interrupt
			if (address == 0xFFFF) // Low Frequancy
			{
				// To do
				return;
			}





			// Unusable Memory
			/*if (InMemoryRange(0xFEA0, 0xFEFF, address)) // Low Frequancy
			{
				m_bus_memory[address] = data;
				return;
			}*/

			// Other uncaught Write commands
			m_bus_memory[address] = data;
		}
	}

	template<class U, class V>
	V& ProcessBusReadRef(U address)
	{
		return m_bus_memory[address];
	}

	typedef void (EmuGB::* OPCfptr) (void);

	OPCfptr m_opCodes[256];
	OPCfptr m_CBOpCodes[256];

	// 8 Bit Registers
	enum class ByteRegisters : ui32
	{
		A_REGISTER = 1,
		F_REGISTER = 0,
		B_REGISTER = 3,
		C_REGISTER = 2,
		D_REGISTER = 5,
		E_REGISTER = 4,
		H_REGISTER = 7,
		L_REGISTER = 6
	};

	enum class WordRegisters : ui32
	{
		AF_REGISTER = 0,
		BC_REGISTER = 1,
		DE_REGISTER = 2,
		HL_REGISTER = 3,
		SP_REGISTER = 4,
		PC_REGISTER = 5
	};

	// Registers
	union
	{
		ui8 m_byte_register[12];
		ui16 m_word_register[6];
	};

	// Flag Register
	/*
	4 : Carry Flag
	5 : Half Carry Flag
	6 : Subtract Flag
	7 : Zero Flag
	*/
	enum class Flags : ui32
	{
		FLAG_ZERO = 7,
		FLAG_SUBTRACT = 6,
		FLAG_HALF_CARRY = 5,
		FLAG_CARRY = 4
	};

	ui8 m_bus_memory[0x10000];

	EmuGBCart m_cartridge;

	// Current opperation
	ui8 m_op_code;

	// Last Cycle
	ui16 m_cycle;

	// Interupt Cycles
	int m_IECycles;

	bool m_interrupts_enabled = false;



	// Total cycles since last v-sync
	ui16 m_frame_cycles;

	ui8 m_cycle_modifier = 0;

	// CPU Cycles
	// Src https://github.com/retrio/gb-test-roms/tree/master/instr_timing
	const ui8 m_cycles[512] = {
		1,3,2,2,1,1,2,1,5,2,2,2,1,1,2,1,
		0,3,2,2,1,1,2,1,3,2,2,2,1,1,2,1,
		2,3,2,2,1,1,2,1,2,2,2,2,1,1,2,1,
		2,3,2,2,3,3,3,1,2,2,2,2,1,1,2,1,
		1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
		1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
		1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
		2,2,2,2,2,2,0,2,1,1,1,1,1,1,2,1,
		1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
		1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
		1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
		1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
		2,3,3,4,3,4,2,4,2,4,3,0,3,6,2,4,
		2,3,3,0,3,4,2,4,2,4,3,0,3,0,2,4,
		3,3,2,0,0,4,2,4,4,1,4,0,0,0,2,4,
		3,3,2,1,0,4,2,4,3,2,4,1,0,0,2,4,

		// cgb
		2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
		2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
		2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
		2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
		2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
		2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
		2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
		2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
		2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
		2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
		2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
		2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
		2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
		2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
		2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
		2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2
	};

	// Instruction Payload
	// Memory that comes directly after the cpu instruction
	const ui8 m_instruction_payload[512] = {
		0, 2, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0, 0, 0, 1, 0, // 0
		0, 2, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, // 1
		1, 2, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, // 2
		1, 2, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, // 3
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 4
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 5
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 6
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 7
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 8
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 9
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // A
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // B
		0, 0, 2, 2, 2, 0, 1, 0, 0, 0, 2, 0, 2, 2, 1, 0, // C
		0, 0, 2, 0, 2, 0, 1, 0, 0, 0, 2, 0, 2, 0, 1, 0, // D
		1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 2, 0, 0, 0, 1, 0, // E
		1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 2, 0, 0, 0, 1, 0, // F

		// Color Game Boy
		// We take account of its own offset + extra
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 1
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 2
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 3
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 4
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 5
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 6
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 7
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 8
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 9
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // A
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // B
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // C
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // D
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // E
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // F

	};

	// Solution found 
	// https://github.com/drhelius/Gearboy/blob/4867b81c27d9b1144f077a20c6e2003ba21bd9a2/src/opcode_timing.h

	const ui8 m_kOPCodeAccurate[512] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 1
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 2
		0, 0, 0, 0, 3, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 3
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 4
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 5
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 6
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 7
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 8
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 9
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // A
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // B
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // C
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // D
		2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, // E
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, // F

		// cb
		0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0, // 0
		0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0, // 1
		0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0, // 2
		0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0, // 3
		0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, // 4
		0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, // 5
		0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, // 6
		0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, // 7
		0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0, // 8
		0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0, // 9
		0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0, // A
		0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0, // B
		0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0, // C
		0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0, // D
		0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0, // E
		0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0  // F
	};

	__forceinline ui8 GetCycleModifier(ui8 cycle);

	__forceinline ui8 ReadByteFromPC();

	__forceinline ui16 ReadWordFromPC();

	__forceinline void ReadWordFromPC(ui16& data);

	template<EmuGB::WordRegisters reg>
	__forceinline void SetWordRegister(ui16 value)
	{
		m_word_register[static_cast<unsigned int>(reg)] = value;
	}

	template<EmuGB::WordRegisters reg>
	__forceinline ui16& GetWordRegister()
	{
		return m_word_register[static_cast<unsigned int>(reg)];
	}


	template<EmuGB::ByteRegisters reg>
	__forceinline ui8& GetByteRegister()
	{
		return m_byte_register[static_cast<unsigned int>(reg)];
	}

	/*
	template<EmuGB::ByteRegisters reg>
	__forceinline void SetByteRegister(ui8 value)
	{
		m_byte_register[static_cast<unsigned int>(reg)] = value;
	}*/

	template<EmuGB::WordRegisters reg>
	__forceinline void StackPop()
	{
		// Used to break up the short into 2 chars
		union
		{
			ui16 data;
			struct
			{
				ui8 low;
				ui8 high;
			}d;
		};
		ProcessBus<MemoryAccessType::Read, ui16, ui8>(GetWordRegister<WordRegisters::SP_REGISTER>(), d.low);
		GetWordRegister<WordRegisters::SP_REGISTER>()++;
		ProcessBus<MemoryAccessType::Read, ui16, ui8>(GetWordRegister<WordRegisters::SP_REGISTER>(), d.high);
		GetWordRegister<WordRegisters::SP_REGISTER>()++;

		SetWordRegister<reg>(data);
	}
	

	template<EmuGB::WordRegisters reg>
	__forceinline void StackPush()
	{
		// Used to break up the short into 2 chars
		union
		{
			ui16 data;
			struct
			{
				ui8 low;
				ui8 high;
			}d;
		};
		data = GetWordRegister<reg>();

		GetWordRegister<WordRegisters::SP_REGISTER>()--;
		ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::SP_REGISTER>(), d.high);
		GetWordRegister<WordRegisters::SP_REGISTER>()--;
		ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::SP_REGISTER>(), d.low);
	}

	template<EmuGB::ByteRegisters reg>
	__forceinline void INCByteRegister()
	{
		bool hasCarry = GetFlag<Flags::FLAG_CARRY>();

		GetByteRegister<ByteRegisters::F_REGISTER>() = 0;

		GetByteRegister<reg>()++;

		SetFlag<Flags::FLAG_CARRY>(hasCarry);

		SetFlag<Flags::FLAG_ZERO>(GetByteRegister<reg>() == 0);

		SetFlag<Flags::FLAG_HALF_CARRY>((GetByteRegister<reg>() & 0x0F) == 0x00);
	}

	template<EmuGB::ByteRegisters reg>
	__forceinline void DECByteRegister()
	{
		GetByteRegister<reg>()--;

		if (!GetFlag<Flags::FLAG_CARRY>())
		{
			GetByteRegister<ByteRegisters::F_REGISTER>() = 0;
		}

		SetFlag<Flags::FLAG_ZERO>(GetByteRegister<reg>() == 0);
		SetFlag<Flags::FLAG_CARRY, true>();
		SetFlag<Flags::FLAG_HALF_CARRY>((GetByteRegister<reg>() & 0x0F) == 0x0F);
	}

	

	template<EmuGB::Flags flag>
	__forceinline bool GetFlag()
	{
		return (GetByteRegister<EmuGB::ByteRegisters::F_REGISTER>() >> static_cast<unsigned int>(flag)) & 1;
	}

	template<EmuGB::Flags flag>
	__forceinline void SetFlag(bool value)
	{
		if (value)
			GetByteRegister<EmuGB::ByteRegisters::F_REGISTER>() |= 1 << static_cast<unsigned int>(flag);
		else
			GetByteRegister<EmuGB::ByteRegisters::F_REGISTER>() &= ~(1 << static_cast<unsigned int>(flag));
	}


	template<EmuGB::Flags flag, bool value>
	__forceinline void SetFlag()
	{
		if constexpr (value)
			GetByteRegister<EmuGB::ByteRegisters::F_REGISTER>() |= 1 << static_cast<unsigned int>(flag);
		else
			GetByteRegister<EmuGB::ByteRegisters::F_REGISTER>() &= ~(1 << static_cast<unsigned int>(flag));
	}


	__forceinline void Jr();

	__forceinline void Bit(const ui8& value, ui8 bit);

	__forceinline void XOR(const ui8& value);

	__forceinline void rl(ui8& value, bool a_reg = false);

	__forceinline void Cp(const ui8& value);

	__forceinline void Sub(const ui8& value);

	//////////////
	// Game Boy //
	//////////////

	__forceinline void Op00(); // Invalid
	__forceinline void Op01(); // LD BC, nn
	__forceinline void Op02(); // LD (BC), A
	__forceinline void Op03(); // INC BC
	__forceinline void Op04(); // INC B
	__forceinline void Op05(); // DEC B
	__forceinline void Op06(); // LD B, n
	__forceinline void Op07(); // RLCA
	__forceinline void Op08(); // LD (nn),SP 
	__forceinline void Op09(); // ADD HL BC
	__forceinline void Op0A(); // LD A, (BC)
	__forceinline void Op0B(); // DEC BC
	__forceinline void Op0C(); // INC C
	__forceinline void Op0D(); // DEC C
	__forceinline void Op0E(); // LD C, n
	__forceinline void Op0F(); // RRCA

	__forceinline void Op10(); // Stop
	__forceinline void Op11(); // LD DE, nn
	__forceinline void Op12(); // LD (DE), A
	__forceinline void Op13(); // INC DE
	__forceinline void Op14(); // INC D
	__forceinline void Op15(); // DEC D
	__forceinline void Op16(); // LD D, n
	__forceinline void Op17(); // RLA
	__forceinline void Op18(); // JR n
	__forceinline void Op19(); // ADD HL, DE
	__forceinline void Op1A(); // LD A, (DE)
	__forceinline void Op1B(); // DEC DE
	__forceinline void Op1C(); // INC E
	__forceinline void Op1D(); // DEC E
	__forceinline void Op1E(); // LD E, n
	__forceinline void Op1F(); // RRA

	__forceinline void Op20(); // JR NZ, n
	__forceinline void Op21(); // LD HL, nn
	__forceinline void Op22(); // LDI (HLI), A
	__forceinline void Op23(); // INC HL
	__forceinline void Op24(); // INC H
	__forceinline void Op25(); // DEC H
	__forceinline void Op26(); // LD H, n
	__forceinline void Op27(); // DAA
	__forceinline void Op28(); // JR Z, n
	__forceinline void Op29(); // ADD HL HL
	__forceinline void Op2A(); // LDI A,(HL)
	__forceinline void Op2B(); // DEC HL
	__forceinline void Op2C(); // INC L
	__forceinline void Op2D(); // DEC L
	__forceinline void Op2E(); // LD L, n
	__forceinline void Op2F(); // CPL

	__forceinline void Op30(); // JR NC, n
	__forceinline void Op31(); // LD SP, nn
	__forceinline void Op32(); // LDD (HL), A
	__forceinline void Op33(); // INC SP
	__forceinline void Op34(); // INC (HL)
	__forceinline void Op35(); // DEC (HL)
	__forceinline void Op36(); // LD (HL),n
	__forceinline void Op37(); // SCF
	__forceinline void Op38(); // JR C, n
	__forceinline void Op39(); // ADD HL SP
	__forceinline void Op3A(); // LD A,(HLD)
	__forceinline void Op3B(); // DEC SP
	__forceinline void Op3C(); // INC A
	__forceinline void Op3D(); // DEC A
	__forceinline void Op3E(); // LD A, #
	__forceinline void Op3F(); // CCF

	__forceinline void Op40(); // LD B, B
	__forceinline void Op41(); // LD B, C
	__forceinline void Op42(); // LD B, D
	__forceinline void Op43(); // LD B, E
	__forceinline void Op44(); // LD B, H
	__forceinline void Op45(); // LD B, L
	__forceinline void Op46(); // LD B, (HL)
	__forceinline void Op47(); // LD B, A
	__forceinline void Op48(); // LD C, B
	__forceinline void Op49(); // LD C, C
	__forceinline void Op4A(); // LD C, D
	__forceinline void Op4B(); // LD C, E
	__forceinline void Op4C(); // LD C, H
	__forceinline void Op4D(); // LD C, L
	__forceinline void Op4E(); // LD C, (HL)
	__forceinline void Op4F(); // LD C, A

	__forceinline void Op50(); // LD D, B
	__forceinline void Op51(); // LD D, C
	__forceinline void Op52(); // LD D, E
	__forceinline void Op53(); // LD D, D
	__forceinline void Op54(); // LD D, H
	__forceinline void Op55(); // LD D, L
	__forceinline void Op56(); // LD D,(HL)
	__forceinline void Op57(); // LD D, A
	__forceinline void Op58(); // LD E, B
	__forceinline void Op59(); // LD E, C
	__forceinline void Op5A(); // LD E, D
	__forceinline void Op5B(); // LD E, E
	__forceinline void Op5C(); // LD E, H
	__forceinline void Op5D(); // LD E, L
	__forceinline void Op5E(); // LD E, (HL)
	__forceinline void Op5F(); // LD E, A

	__forceinline void Op60(); // LD H, B
	__forceinline void Op61(); // LD H, C
	__forceinline void Op62(); // LD H, D
	__forceinline void Op63(); // LD H, E
	__forceinline void Op64(); // LD H, H
	__forceinline void Op65(); // LD H, L
	__forceinline void Op66(); // LD H, (HL)
	__forceinline void Op67(); // LD H, A
	__forceinline void Op68(); // LD L, B
	__forceinline void Op69(); // LD L, C
	__forceinline void Op6A(); // LD L, D
	__forceinline void Op6B(); // LD L, E
	__forceinline void Op6C(); // LD L, H
	__forceinline void Op6D(); // LD L, L
	__forceinline void Op6E(); // LD L, (HL)
	__forceinline void Op6F(); // LD L, A

	__forceinline void Op70(); // LD (HL), B
	__forceinline void Op71(); // LD (HL), C
	__forceinline void Op72(); // LD (HL), D
	__forceinline void Op73(); // LD (HL), E
	__forceinline void Op74(); // LD (HL), H
	__forceinline void Op75(); // LD (HL), L
	__forceinline void Op76(); // HALT
	__forceinline void Op77(); // LD (HL), A
	__forceinline void Op78(); // LD A, B
	__forceinline void Op79(); // LD A, C
	__forceinline void Op7A(); // LD A, D
	__forceinline void Op7B(); // LD A, E
	__forceinline void Op7C(); // LD A, H
	__forceinline void Op7D(); // LD A, L
	__forceinline void Op7E(); // LD A, (HL)
	__forceinline void Op7F(); // LD A, A

	__forceinline void Op80(); // ADD A, B
	__forceinline void Op81(); // ADD A, C
	__forceinline void Op82(); // ADD A, D
	__forceinline void Op83(); // ADD A, E
	__forceinline void Op84(); // ADD A, H
	__forceinline void Op85(); // ADD A, L
	__forceinline void Op86(); // ADD A, (HL)
	__forceinline void Op87(); // ADD A, A
	__forceinline void Op88(); // ADC B
	__forceinline void Op89(); // ADC C
	__forceinline void Op8A(); // ADC D
	__forceinline void Op8B(); // ADC E
	__forceinline void Op8C(); // ADC H
	__forceinline void Op8D(); // ADC L
	__forceinline void Op8E(); // ADC (HL)
	__forceinline void Op8F(); // ADC A

	__forceinline void Op90(); // SUB B
	__forceinline void Op91(); // SUB C
	__forceinline void Op92(); // SUB D
	__forceinline void Op93(); // SUB E
	__forceinline void Op94(); // SUB H
	__forceinline void Op95(); // SUB L
	__forceinline void Op96(); // SUB (HL)
	__forceinline void Op97(); // SUB A
	__forceinline void Op98(); // SBC A, B
	__forceinline void Op99(); // SBC A, C
	__forceinline void Op9A(); // SBC A, D
	__forceinline void Op9B(); // SBC A, E
	__forceinline void Op9C(); // SBC A, H
	__forceinline void Op9D(); // SBC A, L
	__forceinline void Op9E(); // SBC A, (HL)
	__forceinline void Op9F(); // SBC A, A

	__forceinline void OpA0(); // AND B
	__forceinline void OpA1(); // AND C
	__forceinline void OpA2(); // AND D
	__forceinline void OpA3(); // AND E
	__forceinline void OpA4(); // AND H
	__forceinline void OpA5(); // AND L
	__forceinline void OpA6(); // AND (HL)
	__forceinline void OpA7(); // AND A 
	__forceinline void OpA8(); // XOR B
	__forceinline void OpA9(); // XOR C
	__forceinline void OpAA(); // XOR D
	__forceinline void OpAB(); // XOR E
	__forceinline void OpAC(); // XOR H
	__forceinline void OpAD(); // XOR L
	__forceinline void OpAE(); // XOR (HL)
	__forceinline void OpAF(); // XOR A

	__forceinline void OpB0(); // OR B
	__forceinline void OpB1(); // OR C
	__forceinline void OpB2(); // OR D
	__forceinline void OpB3(); // OR E
	__forceinline void OpB4(); // OR H
	__forceinline void OpB5(); // OR L
	__forceinline void OpB6(); // OR (HL)
	__forceinline void OpB7(); // OR A
	__forceinline void OpB8(); // CP B
	__forceinline void OpB9(); // CP C
	__forceinline void OpBA(); // CP D
	__forceinline void OpBB(); // CP E
	__forceinline void OpBC(); // CP H
	__forceinline void OpBD(); // CP L
	__forceinline void OpBE(); // CP (HL)
	__forceinline void OpBF(); // CP A

	__forceinline void OpC0(); // RET NZ
	__forceinline void OpC1(); // POP BC
	__forceinline void OpC2(); // JP NZ,nn
	__forceinline void OpC3(); // JP nn
	__forceinline void OpC4(); // CALL NZ,nn
	__forceinline void OpC5(); // PUSH BC
	__forceinline void OpC6(); // ADD A, #
	__forceinline void OpC7(); // RESET 00
	__forceinline void OpC8(); // RET Z
	__forceinline void OpC9(); // RET
	__forceinline void OpCA(); // JP Z,nn
	//__forceinline void OpCB(); // Game Boy Color Instructions
	__forceinline void OpCC(); // CALL Z,nn
	__forceinline void OpCD(); // CALL nn
	__forceinline void OpCE(); // ADC #
	__forceinline void OpCF(); // RESET 08

	__forceinline void OpD0(); // RET NC
	__forceinline void OpD1(); // POP DE
	__forceinline void OpD2(); // JP NC,nn
	__forceinline void OpD3(); // Invalid
	__forceinline void OpD4(); // CALL NC, nn
	__forceinline void OpD5(); // PUSH DE
	__forceinline void OpD6(); // SUB #
	__forceinline void OpD7(); // RESET 10
	__forceinline void OpD8(); // RET C
	__forceinline void OpD9(); // RETI
	__forceinline void OpDA(); // JP C,nn
	__forceinline void OpDB(); // Invalid
	__forceinline void OpDC(); // CALL C,nn
	__forceinline void OpDD(); // Invalid
	__forceinline void OpDE(); // SBC n
	__forceinline void OpDF(); // RESET 18

	__forceinline void OpE0(); // LDH (n), A
	__forceinline void OpE1(); // POP HL
	__forceinline void OpE2(); // LD (C), A
	__forceinline void OpE3(); // Invalid
	__forceinline void OpE4(); // Invalid
	__forceinline void OpE5(); // PUSH HL
	__forceinline void OpE6(); // AND # 
	__forceinline void OpE7(); // RESET 20
	__forceinline void OpE8(); // ADD SP,n
	__forceinline void OpE9(); // JP HL
	__forceinline void OpEA(); // LD (nn), A
	__forceinline void OpEB(); // Invalid
	__forceinline void OpEC(); // Invalid
	__forceinline void OpED(); // Invalid
	__forceinline void OpEE(); // XOR *
	__forceinline void OpEF(); // RESET 28

	__forceinline void OpF0(); // LDH A, (N)
	__forceinline void OpF1(); // POP AF
	__forceinline void OpF2(); // LD A, FF00 + (C)
	__forceinline void OpF3(); // DI
	__forceinline void OpF4(); // Invalid
	__forceinline void OpF5(); // PUSH AF
	__forceinline void OpF6(); // OR #
	__forceinline void OpF7(); // RESET 30
	__forceinline void OpF8(); // LDHL SP,n 
	__forceinline void OpF9(); // LD SP, HL
	__forceinline void OpFA(); // LD A, (nn)
	__forceinline void OpFB(); // EI
	__forceinline void OpFC(); // Invalid
	__forceinline void OpFD(); // Invalid
	__forceinline void OpFE(); // CP #
	__forceinline void OpFF(); // RESET 38

	////////////////////
	// Game Boy Color //
	////////////////////

	__forceinline void OpCB00(); // RLC B
	__forceinline void OpCB01(); // RLC C
	__forceinline void OpCB02(); // RLC D
	__forceinline void OpCB03(); // RLC E
	__forceinline void OpCB04(); // RLC H
	__forceinline void OpCB05(); // RLC L
	__forceinline void OpCB06(); // RLC (HL)
	__forceinline void OpCB07(); // RLC A
	__forceinline void OpCB08(); // RRC B
	__forceinline void OpCB09(); // RRC C
	__forceinline void OpCB0A(); // RRC D
	__forceinline void OpCB0B(); // RRC E
	__forceinline void OpCB0C(); // RRC H
	__forceinline void OpCB0D(); // RRC L
	__forceinline void OpCB0E(); // RRC (HL)
	__forceinline void OpCB0F(); // RRC A

	__forceinline void OpCB10(); // RL B
	__forceinline void OpCB11(); // RL C
	__forceinline void OpCB12(); // RL D
	__forceinline void OpCB13(); // RL E
	__forceinline void OpCB14(); // RL H
	__forceinline void OpCB15(); // RL L
	__forceinline void OpCB16(); // RL (HL)
	__forceinline void OpCB17(); // RL A
	__forceinline void OpCB18(); // RR B
	__forceinline void OpCB19(); // RR C
	__forceinline void OpCB1A(); // RR D
	__forceinline void OpCB1B(); // RR E
	__forceinline void OpCB1C(); // RR H
	__forceinline void OpCB1D(); // RR L
	__forceinline void OpCB1E(); // RR (HL)
	__forceinline void OpCB1F(); // RR A

	__forceinline void OpCB20(); // SLA B
	__forceinline void OpCB21(); // SLA C
	__forceinline void OpCB22(); // SLA D
	__forceinline void OpCB23(); // SLA E
	__forceinline void OpCB24(); // SLA H
	__forceinline void OpCB25(); // SLA L
	__forceinline void OpCB26(); // SLA (HL)
	__forceinline void OpCB27(); // SLA A
	__forceinline void OpCB28(); // SRA B
	__forceinline void OpCB29(); // SRA C
	__forceinline void OpCB2A(); // SRA D
	__forceinline void OpCB2B(); // SRA E
	__forceinline void OpCB2C(); // SRA H
	__forceinline void OpCB2D(); // SRA L
	__forceinline void OpCB2E(); // SRA (HL)
	__forceinline void OpCB2F(); // SRA A

	__forceinline void OpCB30(); // SWAP B
	__forceinline void OpCB31(); // SWAP C
	__forceinline void OpCB32(); // SWAP D
	__forceinline void OpCB33(); // SWAP E
	__forceinline void OpCB34(); // SWAP H
	__forceinline void OpCB35(); // SWAP L
	__forceinline void OpCB36(); // SWAP (HL)
	__forceinline void OpCB37(); // SWAP A
	__forceinline void OpCB38(); // SRL B
	__forceinline void OpCB39(); // SRL C
	__forceinline void OpCB3A(); // SRL D
	__forceinline void OpCB3B(); // SRL E
	__forceinline void OpCB3C(); // SRL H
	__forceinline void OpCB3D(); // SRL L
	__forceinline void OpCB3E(); // SRL (HL)
	__forceinline void OpCB3F(); // SRL A

	__forceinline void OpCB40(); // BIT 0, B
	__forceinline void OpCB41(); // BIT 0, C
	__forceinline void OpCB42(); // BIT 0, D
	__forceinline void OpCB43(); // BIT 0, E
	__forceinline void OpCB44(); // BIT 0, H
	__forceinline void OpCB45(); // BIT 0, L
	__forceinline void OpCB46(); // BIT 0, (HL)
	__forceinline void OpCB47(); // BIT 0, A
	__forceinline void OpCB48(); // BIT 1, B
	__forceinline void OpCB49(); // BIT 1, C
	__forceinline void OpCB4A(); // BIT 1, D
	__forceinline void OpCB4B(); // BIT 1, E
	__forceinline void OpCB4C(); // BIT 1, H
	__forceinline void OpCB4D(); // BIT 1, L
	__forceinline void OpCB4E(); // BIT 1, (HL)
	__forceinline void OpCB4F(); // BIT 1, A

	__forceinline void OpCB50(); // BIT 2, B
	__forceinline void OpCB51(); // BIT 2, C
	__forceinline void OpCB52(); // BIT 2, D
	__forceinline void OpCB53(); // BIT 2, E
	__forceinline void OpCB54(); // BIT 2, H
	__forceinline void OpCB55(); // BIT 2, L
	__forceinline void OpCB56(); // BIT 2, (HL)
	__forceinline void OpCB57(); // BIT 2, A
	__forceinline void OpCB58(); // BIT 3, B
	__forceinline void OpCB59(); // BIT 3, C
	__forceinline void OpCB5A(); // BIT 3, D
	__forceinline void OpCB5B(); // BIT 3, E
	__forceinline void OpCB5C(); // BIT 3, H
	__forceinline void OpCB5D(); // BIT 3, L
	__forceinline void OpCB5E(); // BIT 3, (HL)
	__forceinline void OpCB5F(); // BIT 3, A

	__forceinline void OpCB60(); // BIT 4, B
	__forceinline void OpCB61(); // BIT 4, C
	__forceinline void OpCB62(); // BIT 4, D
	__forceinline void OpCB63(); // BIT 4, E
	__forceinline void OpCB64(); // BIT 4, H
	__forceinline void OpCB65(); // BIT 4, L
	__forceinline void OpCB66(); // BIT 4, (HL)
	__forceinline void OpCB67(); // BIT 4, A
	__forceinline void OpCB68(); // BIT 5, B
	__forceinline void OpCB69(); // BIT 5, C
	__forceinline void OpCB6A(); // BIT 5, D
	__forceinline void OpCB6B(); // BIT 5, E
	__forceinline void OpCB6C(); // BIT 5, H
	__forceinline void OpCB6D(); // BIT 5, L
	__forceinline void OpCB6E(); // BIT 5, (HL)
	__forceinline void OpCB6F(); // BIT 5, A

	__forceinline void OpCB70(); // BIT 6, B
	__forceinline void OpCB71(); // BIT 6, C
	__forceinline void OpCB72(); // BIT 6, D
	__forceinline void OpCB73(); // BIT 6, E
	__forceinline void OpCB74(); // BIT 6, H
	__forceinline void OpCB75(); // BIT 6, L
	__forceinline void OpCB76(); // BIT 6, (HL)
	__forceinline void OpCB77(); // BIT 6, A
	__forceinline void OpCB78(); // BIT 7, B
	__forceinline void OpCB79(); // BIT 7, C
	__forceinline void OpCB7A(); // BIT 7, D
	__forceinline void OpCB7B(); // BIT 7, E
	__forceinline void OpCB7C(); // BIT 7, H
	__forceinline void OpCB7D(); // BIT 7, L
	__forceinline void OpCB7E(); // BIT 7, (HL)
	__forceinline void OpCB7F(); // BIT 7, A

	__forceinline void OpCB80(); // RES 0, B
	__forceinline void OpCB81(); // RES 0, C
	__forceinline void OpCB82(); // RES 0, D
	__forceinline void OpCB83(); // RES 0, E
	__forceinline void OpCB84(); // RES 0, H
	__forceinline void OpCB85(); // RES 0, L
	__forceinline void OpCB86(); // RES 0,(HL)
	__forceinline void OpCB87(); // RES 0, A
	__forceinline void OpCB88(); // RES 1, B
	__forceinline void OpCB89(); // RES 1, C
	__forceinline void OpCB8A(); // RES 1, D
	__forceinline void OpCB8B(); // RES 1, E
	__forceinline void OpCB8C(); // RES 1, H
	__forceinline void OpCB8D(); // RES 1, L
	__forceinline void OpCB8E(); // RES 1,(HL)
	__forceinline void OpCB8F(); // RES 1, A

	__forceinline void OpCB90(); // RES 2, B
	__forceinline void OpCB91(); // RES 2, C
	__forceinline void OpCB92(); // RES 2, D
	__forceinline void OpCB93(); // RES 2, E
	__forceinline void OpCB94(); // RES 2, H
	__forceinline void OpCB95(); // RES 2, L
	__forceinline void OpCB96(); // RES 2,(HL)
	__forceinline void OpCB97(); // RES 2, A
	__forceinline void OpCB98(); // RES 3, B
	__forceinline void OpCB99(); // RES 3, C
	__forceinline void OpCB9A(); // RES 3, D
	__forceinline void OpCB9B(); // RES 3, E
	__forceinline void OpCB9C(); // RES 3, H
	__forceinline void OpCB9D(); // RES 3, L
	__forceinline void OpCB9E(); // RES 3,(HL)
	__forceinline void OpCB9F(); // RES 3, A

	__forceinline void OpCBA0(); // RES 4, B
	__forceinline void OpCBA1(); // RES 4, C
	__forceinline void OpCBA2(); // RES 4, D
	__forceinline void OpCBA3(); // RES 4, E
	__forceinline void OpCBA4(); // RES 4, H
	__forceinline void OpCBA5(); // RES 4, L
	__forceinline void OpCBA6(); // RES 4,(HL)
	__forceinline void OpCBA7(); // RES 4, A
	__forceinline void OpCBA8(); // RES 5, B
	__forceinline void OpCBA9(); // RES 5, C
	__forceinline void OpCBAA(); // RES 5, D
	__forceinline void OpCBAB(); // RES 5, E
	__forceinline void OpCBAC(); // RES 5, H
	__forceinline void OpCBAD(); // RES 5, L
	__forceinline void OpCBAE(); // RES 5,(HL)
	__forceinline void OpCBAF(); // RES 5, A

	__forceinline void OpCBB0(); // RES 6, B
	__forceinline void OpCBB1(); // RES 6, C
	__forceinline void OpCBB2(); // RES 6, D
	__forceinline void OpCBB3(); // RES 6, E
	__forceinline void OpCBB4(); // RES 6, H
	__forceinline void OpCBB5(); // RES 6, L
	__forceinline void OpCBB6(); // RES 6,(HL)
	__forceinline void OpCBB7(); // RES 6, A
	__forceinline void OpCBB8(); // RES 7, B
	__forceinline void OpCBB9(); // RES 7, C
	__forceinline void OpCBBA(); // RES 7, D
	__forceinline void OpCBBB(); // RES 7, E
	__forceinline void OpCBBC(); // RES 7, H
	__forceinline void OpCBBD(); // RES 7, L
	__forceinline void OpCBBE(); // RES 7,(HL)
	__forceinline void OpCBBF(); // RES 7, A

	__forceinline void OpCBC0(); // SET 0, B
	__forceinline void OpCBC1(); // SET 0, C
	__forceinline void OpCBC2(); // SET 0, D
	__forceinline void OpCBC3(); // SET 0, E
	__forceinline void OpCBC4(); // SET 0, H
	__forceinline void OpCBC5(); // SET 0, L
	__forceinline void OpCBC6(); // SET 0,(HL)
	__forceinline void OpCBC7(); // SET 0, A
	__forceinline void OpCBC8(); // SET 1, B
	__forceinline void OpCBC9(); // SET 1, C
	__forceinline void OpCBCA(); // SET 1, D
	__forceinline void OpCBCB(); // SET 1, E
	__forceinline void OpCBCC(); // SET 1, H
	__forceinline void OpCBCD(); // SET 1, L
	__forceinline void OpCBCE(); // SET 1,(HL)
	__forceinline void OpCBCF(); // SET 1, A

	__forceinline void OpCBD0(); // SET 2, B
	__forceinline void OpCBD1(); // SET 2, C
	__forceinline void OpCBD2(); // SET 2, D
	__forceinline void OpCBD3(); // SET 2, E
	__forceinline void OpCBD4(); // SET 2, H
	__forceinline void OpCBD5(); // SET 2, L
	__forceinline void OpCBD6(); // SET 2,(HL)
	__forceinline void OpCBD7(); // SET 2, A
	__forceinline void OpCBD8(); // SET 3, B
	__forceinline void OpCBD9(); // SET 3, C
	__forceinline void OpCBDA(); // SET 3, D
	__forceinline void OpCBDB(); // SET 3, E
	__forceinline void OpCBDC(); // SET 3, H
	__forceinline void OpCBDD(); // SET 3, L
	__forceinline void OpCBDE(); // SET 3,(HL)
	__forceinline void OpCBDF(); // SET 3, A

	__forceinline void OpCBE0(); // SET 4, B
	__forceinline void OpCBE1(); // SET 4, C
	__forceinline void OpCBE2(); // SET 4, D
	__forceinline void OpCBE3(); // SET 4, E
	__forceinline void OpCBE4(); // SET 4, H
	__forceinline void OpCBE5(); // SET 4, L
	__forceinline void OpCBE6(); // SET 4,(HL)
	__forceinline void OpCBE7(); // SET 4, A
	__forceinline void OpCBE8(); // SET 5, B
	__forceinline void OpCBE9(); // SET 5, C
	__forceinline void OpCBEA(); // SET 5, D
	__forceinline void OpCBEB(); // SET 5, E
	__forceinline void OpCBEC(); // SET 5, H
	__forceinline void OpCBED(); // SET 5, L
	__forceinline void OpCBEE(); // SET 5,(HL)
	__forceinline void OpCBEF(); // SET 5, A

	__forceinline void OpCBF0(); // SET 6, B
	__forceinline void OpCBF1(); // SET 6, C
	__forceinline void OpCBF2(); // SET 6, D
	__forceinline void OpCBF3(); // SET 6, E
	__forceinline void OpCBF4(); // SET 6, H
	__forceinline void OpCBF5(); // SET 6, L
	__forceinline void OpCBF6(); // SET 6,(HL)
	__forceinline void OpCBF7(); // SET 6, A
	__forceinline void OpCBF8(); // SET 7, B
	__forceinline void OpCBF9(); // SET 7, C
	__forceinline void OpCBFA(); // SET 7, D
	__forceinline void OpCBFB(); // SET 7, E
	__forceinline void OpCBFC(); // SET 7, H
	__forceinline void OpCBFD(); // SET 7, L
	__forceinline void OpCBFE(); // SET 7,(HL)
	__forceinline void OpCBFF(); // SET 7, A
};