#pragma once

#include <memory>

#include <Base.hpp>
#include <Definitions.hpp>

enum Register : ui8
{
	// Stays Zero
	zero,
	// Reserved for the assembler
	at,
	// Values for results and expression evaluation	 
	v0,
	v1,
	// Arguments
	a0,
	a1,
	a2,
	a3,
	// Temporaries (not preserved across call)	
	t0,
	t1,
	t2,
	t3,
	t4,
	t5,
	t6,
	t7,
	// Saved (preserved across call)	
	s0,
	s1,
	s2,
	s3,
	s4,
	s5,
	s6,
	s7,
	// More temporaries (not preserved across call)
	t8,
	t9,
	// Reserved for OS Kernel
	k0,
	k1,
	// Global Pointer
	gp,
	// Stack Pointer
	sp,
	// Frame Pointer
	fp,
	// Return Address
	ra,

	// Both read as a 64 bit number
	// Multiplication 64 bit high result or division  remainder
	hi,
	// Multiplication 64 bit low result or division quotient
	lo,

	// Program counter
	pc,
	// Next Program counter
	npc,
	// Total register count
	MAX
};

enum InstructionOp : ui8
{

	function = 0, // Function
	b        = 1,
	j        = 2,  // Jump
	jal      = 3,
	beq      = 4,
	bne      = 5,
	blez     = 6,
	bgtz     = 7,
	addi     = 8,
	addiu    = 9,  // Add Immediate Unsigned
	slti     = 10,
	sltiu    = 11,
	andi     = 12,
	ori      = 13, // OR immidiate
	xori     = 14,
	lui      = 15, // Load upper immidiate
	cop0     = 16,
	cop1     = 17,
	cop2     = 18,
	cop3     = 19,
	lb       = 32, 
	lh       = 33, 
	lwl      = 34,
	lw       = 35,
	lbu      = 36,
	lhu      = 37,
	lwr      = 38,
	sb       = 40,
	sh       = 41,
	swl      = 42,
	sw       = 43, // Store Word
	swr      = 46,
	lwc0     = 48,
	lwc1     = 49,
	lwc2     = 50,
	lwc3     = 51,
	swc0     = 56,
	swc1     = 57,
	swc2     = 58,
	swc3     = 59
};

// All internal cpu registers
struct Registers
{
	union
	{
		struct
		{
			// Stays Zero
			ui32 zero;
			// Reserved for the assembler
			ui32 at;
			// Values for results and expression evaluation	 
			ui32 v0;
			ui32 v1;
			// Arguments
			ui32 a0;
			ui32 a1;
			ui32 a2;
			ui32 a3;
			// Temporaries (not preserved across call)	
			ui32 t0;
			ui32 t1;
			ui32 t2;
			ui32 t3;
			ui32 t4;
			ui32 t5;
			ui32 t6;
			ui32 t7;
			// Saved (preserved across call)	
			ui32 s0;
			ui32 s1;
			ui32 s2;
			ui32 s3;
			ui32 s4;
			ui32 s5;
			ui32 s6;
			ui32 s7;
			// More temporaries (not preserved across call)
			ui32 t8;
			ui32 t9;
			// Reserved for OS Kernel
			ui32 k0;
			ui32 k1;
			// Global Pointer
			ui32 gp;
			// Stack Pointer
			ui32 sp;
			// Frame Pointer
			ui32 fp;
			// Return Address
			ui32 ra;

			// Both read as a 64 bit number
			// Multiplication 64 bit high result or division  remainder
			ui32 hi;
			// Multiplication 64 bit low result or division quotient
			ui32 lo;
			// Program counter
			ui32 pc;
			// Next Program counter
			ui32 npc;
		};
		ui32 data[Register::MAX];
	};
};

enum class Exception : ui8
{
	INT = 0x00,     // Interrupt
	MOD = 0x01,     // TLB Modification
	TLBL = 0x02,    // TLB Load
	TLBS = 0x03,    // TLB Store
	AdEL = 0x04,    // Address error, data load/instruction fetch
	AdES = 0x05,    // Address error, data store
	IBE = 0x06,     // Bus error on instruction fetch
	DBE = 0x07,     // Bus error on data load/store
	Syscall = 0x08, // System call instruction
	BP = 0x09,      // Break instruction
	RI = 0x0A,      // Reserved instruction
	CpU = 0x0B,     // Coprocessor unusable
	Ov = 0x0C,      // Arithmetic overflow
};


enum InstructionFunction : ui8
{

	sll = 0, // Shift Left Logical
	srl = 2, // Shift right logical
	sra = 3, // Shift right arithmaetic 
	sllv = 4, // Shift left logical value

	srlv = 6, // Shift right logical variable
	srav = 7, // Shift right arithmaetic
	jr = 8, // Jump Register
	jalr = 9, // Jump and link register

	syscall = 12, // System call
	break_ = 13, // Break

	mfhi = 16, // Move from high
	mthi = 17, // Move to high
	mflo = 18, // Move from low
	mtlo = 19, // Move to low

	mult = 24, // Multiply
	multu = 25, // Multiply Unsigned
	div_ = 26, // Divide
	divu = 27, // Divide unsigned

	add = 32, // Add
	addu = 33, // Add Unsigned
	sub = 34, // Subtract
	subu = 35,// Subtract unsigned
	and_ = 36,
	or_ = 37,
	xor_ = 38,
	nor = 39, // Not or

	slt = 42, // Set on less than
	sltu = 43, // Set on less than unsigned
};

template<typename TReturn, typename TValue>
__forceinline constexpr TReturn ZeroExtend( TValue value )
{
	return static_cast<TReturn>(static_cast<typename std::make_unsigned<TReturn>::type>(
		static_cast<typename std::make_unsigned<TValue>::type>(value)));
}

// Sign-extending helper
template<typename TReturn, typename TValue>
__forceinline constexpr TReturn SignExtend( TValue value )
{
	return static_cast<TReturn>(
		static_cast<typename std::make_signed<TReturn>::type>(static_cast<typename std::make_signed<TValue>::type>(value)));
}

template<typename TValue>
__forceinline constexpr ui32 ZeroExtend32( TValue value )
{
	return ZeroExtend<ui32, TValue>( value );
}

template<typename TValue>
__forceinline constexpr i64 ZeroExtend64( TValue value )
{
	return ZeroExtend<ui64, TValue>( value );
}

template<typename TValue>
__forceinline constexpr i32 SignExtend32( TValue value )
{
	return SignExtend<i32, TValue>( value );
}

template<typename TValue>
__forceinline constexpr ui64 SignExtend64( TValue value )
{
	return SignExtend<ui64, TValue>( value );
}

template<class origionalDataType, class fieldDataType, unsigned offset, unsigned size>
struct BitField
{
public:
	origionalDataType data;

	__forceinline constexpr origionalDataType GetMask() const
	{
		// Calculate how many bits are in the data type
		static constexpr unsigned int dataSize = sizeof( origionalDataType ) * 8;
		// Bitshift the value to the right by x and then to the left by y to give final mask
		return ((static_cast<origionalDataType>(~0) >> (dataSize - size)) << offset);
	}

	__forceinline BitField& operator=( fieldDataType value )
	{
		Set( value );
		return *this;
	}

	__forceinline BitField& operator<<=( fieldDataType rhs )
	{
		Set( Get() << rhs );
		return *this;
	}

	__forceinline void Set( fieldDataType value )
	{
		data = (data & ~GetMask()) | ((static_cast<origionalDataType>(value) << offset) & GetMask());
	}

	__forceinline fieldDataType Get() const
	{
		return static_cast<fieldDataType>((data & GetMask()) >> offset);
	}
	__forceinline operator fieldDataType() const { return Get(); }
};


union Instruction
{
	ui32 data;

	BitField<ui32, ui8, 26, 6> op;

	union
	{
		BitField<ui32, Register, 21, 5> rs;   // Src Register
		BitField<ui32, Register, 16, 5> rt;   // Target Register
		BitField<ui32, ui16, 0, 16> imm;      // Immediate	

		__forceinline i32 immAsI32() const { return SignExtend32( imm.Get() ); }
		__forceinline ui32 immAsUI32() const { return ZeroExtend32( imm.Get() ); }
	} i;

	union
	{
		BitField<ui32, ui32, 0, 26> target;                 // Jump Target
	} j;

	union
	{
		BitField<ui32, Register, 21, 5> rs;                      // Src Register
		BitField<ui32, Register, 16, 5> rt;                      // Target Register
		BitField<ui32, Register, 11, 5> rd;                      // Dst Register
		BitField<ui32, ui8, 6, 5> shamt;                    // Shift amount
		BitField<ui32, InstructionFunction, 0, 6> function; // Function Identifier
	} r;


};

struct EmuPSX : public EmuBase<EmuPSX>
{
	friend struct EmuBase<EmuPSX>;
	EmuPSX();
private:
	bool InitEmu( const char* path );

	void TickEmu();

	void KeyPressEmu( ConsoleKeys key );

	void KeyReleaseEmu( ConsoleKeys key );

	bool IsKeyDownEmu( ConsoleKeys key );

	unsigned int ScreenWidthEmu();

	unsigned int ScreenHeightEmu();

	void GetScreenBufferEmu( char*& ptr, unsigned int& size );

	__forceinline ui32& GetRegister( Register reg);

	void LoadBIOS();

	void GetNextInstruction();

	void ExecuteInstruction();

	template<MemoryAccessType type, class T>
	void ProcessDispatch( ui32 address, std::conditional_t<type == MemoryAccessType::Read, ui32&, ui32> data );

	template<MemoryAccessType type, class T>
	void ProcessBIOSDispatch( ui32 address, std::conditional_t<type == MemoryAccessType::Read, ui32&, ui32> data );

	template<class T>
	bool AlignmentCheck( ui32 address );

	template<MemoryAccessType type, class T>
	bool AlignmentCheck( ui32 address );

	bool InMemoryRange( ui32 start, ui32 end, ui32 data );

	void WriteRegister( ui8 writeRegister, ui32 value );

	ui32 ReadRegister( ui8 readRegister );

	void WriteMemoryWord( ui32 address, ui32 value );

	void TakeBranch( ui32 address );



	void RaiseException( Exception ex );

	void RaiseException( Exception ex, ui32 cPC, bool inBranchDelaySlot, bool currentInstructionBranchTaken, ui8 ce );

	Registers mRegisters;

	Instruction mNextInstruction;

	Instruction mCurrentInstruction;


	bool mExceptionThrown;

	bool mCurrentInstructionIsBranchDelaySlot;
	bool mNextInstructionIsBranchDelaySlot;
	bool mCurrentBranchTaken;
	bool mNextBranchTaken;


	// PSX Memory Map
	// KUSEG      KSEG0      KSEG1      Length Description
	// 0x00000000 0x80000000 0xa0000000 2048K Main RAM
	// 0x1f000000 0x9f000000 0xbf000000 8192K Expansion Region 1
	// 0x1f800000 0x9f800000 0xbf800000 1K Scratchpad
	// 0x1f801000 0x9f801000 0xbf801000 8K Hardware registers
	// 0x1fc00000 0x9fc00000 0xbfc00000 512K BIOS ROM
	std::unique_ptr<ui8[]> mBIOS;


	const unsigned int BIOS_START = 0xBFC00000;
	const unsigned int BIOS_LENGTH = 512 * 1024;



	static const unsigned int RAM_BASE = 0x00000000;
	static const unsigned int RAM_SIZE = 0x200000;


	static const unsigned int TotalRamSize = 0x800000;
	static const unsigned int EXP1_BASE = 0x1F000000;
	static const unsigned int EXP1_SIZE = 0x800000;

	// Memory Controll Space
	static const unsigned int MEMCTRL_BASE = 0x1F801000;
	static const unsigned int MEMCTRL_SIZE = 0x40;
	static const unsigned int PAD_BASE = 0x1F801040;
	static const unsigned int PAD_SIZE = 0x10;
	static const unsigned int PAD_MASK = PAD_SIZE - 1;
	static const unsigned int SIO_BASE = 0x1F801050;
	static const unsigned int SIO_SIZE = 0x10;

	static const unsigned int MEMCTRL2_BASE = 0x1F801060;
	static const unsigned int MEMCTRL2_SIZE = 0x10;
	static const unsigned int INTERRUPT_CONTROLLER_BASE = 0x1F801070;
	static const unsigned int INTERRUPT_CONTROLLER_SIZE = 0x10;
	// Direct memory access, use for transfering texture data etc without the cpu
	static const unsigned int DMA_BASE = 0x1F801080;
	static const unsigned int DMA_SIZE = 0x80;

	static const unsigned int TIMERS_BASE = 0x1F801100;
	static const unsigned int TIMERS_SIZE = 0x40;
	static const unsigned int CDROM_BASE = 0x1F801800;
	static const unsigned int CDROM_SIZE = 0x10;
	static const unsigned int GPU_BASE = 0x1F801810;
	static const unsigned int GPU_SIZE = 0x10;
	static const unsigned int GPU_MASK = GPU_SIZE - 1;
	static const unsigned int MDEC_BASE = 0x1F801820;
	static const unsigned int MDEC_SIZE = 0x10;
	static const unsigned int SPU_BASE = 0x1F801C00;
	static const unsigned int SPU_SIZE = 0x300;
	static const unsigned int EXP2_BASE = 0x1F802000;
	static const unsigned int EXP2_SIZE = 0x2000;

};




template<MemoryAccessType type, class T>
inline void EmuPSX::ProcessDispatch( ui32 address, std::conditional_t<type == MemoryAccessType::Read, ui32&, ui32> data )
{
	static constexpr int dataSize = sizeof( T );
	if (address < TotalRamSize)
	{
		std::cout << "Uninplemented Memory 0x" << std::hex << address << std::endl;
		throw("Uninplemented");
	}
	else if (address < EXP1_BASE)
	{
		std::cout << "Uninplemented Memory 0x" << std::hex << address << std::endl;
		throw("Uninplemented");
	}
	else if (address < (EXP1_BASE + EXP1_SIZE))
	{
		std::cout << "Uninplemented Memory 0x" << std::hex << address << std::endl;
		throw("Uninplemented");
	}
	else if (address < MEMCTRL_BASE)
	{
		std::cout << "Uninplemented Memory 0x" << std::hex << address << std::endl;
		throw("Uninplemented");
	}
	else if (address < (MEMCTRL_BASE + MEMCTRL_SIZE))
	{
		//throw("Uninplemented");
		// Inplement memory controll later
	}
	else if (address < (PAD_BASE + PAD_SIZE))
	{
		std::cout << "Uninplemented Memory 0x" << std::hex << address << std::endl;
		throw("Uninplemented");
	}
	else if (address < (SIO_BASE + SIO_SIZE))
	{
		std::cout << "Uninplemented Memory 0x" << std::hex << address << std::endl;
		throw("Uninplemented");
	}
	else if (address < (MEMCTRL2_BASE + MEMCTRL2_SIZE))
	{
		// Not inplemented yet

		//std::cout << "Uninplemented Memory 0x" << std::hex << address << std::endl;
		//throw("Uninplemented");
	}
	else if (address < (INTERRUPT_CONTROLLER_BASE + INTERRUPT_CONTROLLER_SIZE))
	{
		std::cout << "Uninplemented Memory 0x" << std::hex << address << std::endl;
		throw("Uninplemented");
	}
	else if (address < (DMA_BASE + DMA_SIZE))
	{
		std::cout << "Uninplemented Memory 0x" << std::hex << address << std::endl;
		throw("Uninplemented");
	}
	else if (address < (TIMERS_BASE + TIMERS_SIZE))
	{
		std::cout << "Uninplemented Memory 0x" << std::hex << address << std::endl;
		throw("Uninplemented");
	}
	else if (address < CDROM_BASE)
	{
		std::cout << "Uninplemented Memory 0x" << std::hex << address << std::endl;
		throw("Uninplemented");
	}
	else if (address < (CDROM_BASE + CDROM_SIZE))
	{
		std::cout << "Uninplemented Memory 0x" << std::hex << address << std::endl;
		throw("Uninplemented");
	}
	else if (address < (GPU_BASE + GPU_SIZE))
	{
		std::cout << "Uninplemented Memory 0x" << std::hex << address << std::endl;
		throw("Uninplemented");
	}
	else if (address < (MDEC_BASE + MDEC_SIZE))
	{
		std::cout << "Uninplemented Memory 0x" << std::hex << address << std::endl;
		throw("Uninplemented");
	}
	else if (address < SPU_BASE)
	{
		std::cout << "Uninplemented Memory 0x" << std::hex << address << std::endl;
		throw("Uninplemented");
	}
	else if (address < (SPU_BASE + SPU_SIZE))
	{
		std::cout << "Uninplemented Memory 0x" << std::hex << address << std::endl;
		throw("Uninplemented");
	}
	else if (address < EXP2_BASE)
	{
		std::cout << "Uninplemented Memory 0x" << std::hex << address << std::endl;
		throw("Uninplemented");
	}
	else if (address < (EXP2_BASE + EXP2_SIZE))
	{
		std::cout << "Uninplemented Memory 0x" << std::hex << address << std::endl;
		throw("Uninplemented");
	}
	else if (address < BIOS_START)
	{
		std::cout << "Uninplemented Memory 0x" << std::hex << address << std::endl;
		throw("Uninplemented");
	}
	// BIOS
	else if (address < BIOS_START + BIOS_LENGTH)
	{
		ProcessBIOSDispatch<type, T>( address, data );
	}
	else
	{
		throw("Invalid Address");
	}



}

template<MemoryAccessType type, class T>
inline void EmuPSX::ProcessBIOSDispatch( ui32 address, std::conditional_t<type == MemoryAccessType::Read, ui32&, ui32> data )
{

	address -= BIOS_START;

	static constexpr int dataSize = sizeof( T );

	if constexpr (type == MemoryAccessType::Read)
	{
		if constexpr (dataSize == 1) // ui8
		{
			data = ZeroExtend32( mBIOS[address] );
		}
		else if constexpr (dataSize == 2) // ui16
		{
			ui16 tempData = *reinterpret_cast<ui16*>(&mBIOS[address]);

			data = ZeroExtend32( tempData );
		}
		else // ui32
		{
			data = *reinterpret_cast<ui32*>(&mBIOS[address]);
		}
	}
	else if constexpr (type == MemoryAccessType::Write)
	{
		// Do nothing for wrights
	}
}

template<class T>
inline bool EmuPSX::AlignmentCheck( ui32 address )
{
	static constexpr int dataSize = sizeof( T );
	return address % dataSize == 0;
}

template<MemoryAccessType type, class T>
inline bool EmuPSX::AlignmentCheck( ui32 address )
{
	bool result = AlignmentCheck<T>( address );
	if (result) return true;

	// Raise PSX exception

	return false;
}