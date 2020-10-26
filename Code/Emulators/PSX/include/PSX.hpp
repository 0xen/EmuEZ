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


enum class Cop0Reg : ui8
{
	BPC = 3, // Break point Exception
	BDA = 5, // Data Break point
	JUMPDEST = 6, // Not sure
	DCIC = 7, // Enable / Dissable heardware break points
	BadVaddr = 8,
	BDAM = 9, // Bitmask applied to BDA
	BPCM = 11, // Bitmask for BPC
	SR = 12, // Status Register
	CAUSE = 13, // Read only data describing an exception
	EPC = 14,
	PRID = 15
};

enum InstructionOp : ui8
{

	function = 0, // Function
	b        = 1,
	j        = 2,  // Jump
	jal      = 3,  // Jump and link
	beq      = 4,
	bne      = 5,  // Branch not equals
	blez     = 6,
	bgtz     = 7,
	addi     = 8,  // Add Immediate Signed
	addiu    = 9,  // Add Immediate Unsigned
	slti     = 10,
	sltiu    = 11,
	andi     = 12, // Bitwise and immediate
	ori      = 13, // OR immidiate
	xori     = 14,
	lui      = 15, // Load upper immidiate
	cop0     = 16, // Co processor 0
	cop1     = 17,
	cop2     = 18,
	cop3     = 19,
	lb       = 32, 
	lh       = 33, 
	lwl      = 34,
	lw       = 35, // Load Word
	lbu      = 36,
	lhu      = 37,
	lwr      = 38,
	sb       = 40, // Store byte
	sh       = 41, // Store Halfword
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

enum class Cop0Instruction : ui32
{
	tlbr = 0x01,
	tlbwi = 0x02,
	tlbwr = 0x04,
	tlbp = 0x08,
	rfe = 0x10,
};

enum class CopCommonInstruction : ui32
{
	mfcn = 0b0000,
	cfcn = 0b0010,
	mtcn = 0b0100, // Move to co-processor 'n'
	ctcn = 0b0110,
	bcnc = 0b1000,
};

template<typename TValue>
__forceinline constexpr ui8 Truncate8( TValue value )
{
	return static_cast<ui8>(static_cast<typename std::make_unsigned<decltype(value)>::type>(value));
}
template<typename TValue>
__forceinline constexpr ui16 Truncate16( TValue value )
{
	return static_cast<ui16>(static_cast<typename std::make_unsigned<decltype(value)>::type>(value));
}
template<typename TValue>
__forceinline constexpr ui32 Truncate32( TValue value )
{
	return static_cast<ui32>(static_cast<typename std::make_unsigned<decltype(value)>::type>(value));
}

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


	union
	{
		ui32 bits;
		BitField<ui32, ui8, 26, 2> cop_n;
		BitField<ui32, ui16, 0, 16> imm16;
		BitField<ui32, ui32, 0, 25> imm25;

		__forceinline Cop0Instruction Cop0Op() const { return static_cast<Cop0Instruction>(bits & UINT32_C( 0x3F )); }

		__forceinline bool IsCommonInstruction() const { return (bits & (UINT32_C( 1 ) << 25)) == 0; }

		__forceinline CopCommonInstruction CommonOp() const
		{
			return static_cast<CopCommonInstruction>((bits >> 21) & INT32_C( 0b1111 ));
		}
	} cop;

};


// Cop register solution found here https://github.com/stenzek/duckstation/blob/master/src/core/cpu_types.h
struct Cop0Registers
{
	ui32 BPC;      // Breakpoint on execute
	ui32 BDA;      // Breakpoint on data access
	ui32 TAR;      // Randomly memorized jump address
	ui32 BadVaddr; // Bad virtual address value
	ui32 BDAM;     // Data breakpoint mask
	ui32 BPCM;     // Execute breakpoint mask
	ui32 EPC;      // Return address from trap
	ui32 PRID;     // Processor ID

	union SR
	{
		ui32 bits;
		BitField<ui32, bool, 0, 1> IEc;  // current interrupt enable
		BitField<ui32, bool, 1, 1> KUc;  // current kernel/user mode, user = 1
		BitField<ui32, bool, 2, 1> IEp;  // previous interrupt enable
		BitField<ui32, bool, 3, 1> KUp;  // previous kernel/user mode, user = 1
		BitField<ui32, bool, 4, 1> IEo;  // old interrupt enable
		BitField<ui32, bool, 5, 1> KUo;  // old kernel/user mode, user = 1
		BitField<ui32, ui8, 8, 8> Im;     // interrupt mask, set to 1 = allowed to trigger
		BitField<ui32, bool, 16, 1> Isc; // isolate cache, no writes to memory occur
		BitField<ui32, bool, 17, 1> Swc; // swap data and instruction caches
		BitField<ui32, bool, 18, 1> PZ;  // zero cache parity bits
		BitField<ui32, bool, 19, 1> CM;  // last isolated load contains data from memory (tag matches?)
		BitField<ui32, bool, 20, 1> PE;  // cache parity error
		BitField<ui32, bool, 21, 1> TS;  // tlb shutdown - matched two entries
		BitField<ui32, bool, 22, 1> BEV; // boot exception vectors, 0 = KSEG0, 1 = KSEG1
		BitField<ui32, bool, 25, 1> RE;  // reverse endianness in user mode
		BitField<ui32, bool, 28, 1> CU0; // coprocessor 0 enable in user mode
		BitField<ui32, bool, 29, 1> CU1; // coprocessor 1 enable in user mode
		BitField<ui32, bool, 30, 1> CU2; // coprocessor 2 enable in user mode
		BitField<ui32, bool, 31, 1> CU3; // coprocessor 3 enable in user mode

		BitField<ui32, ui8, 0, 6> mode_bits;
		BitField<ui32, ui8, 28, 2> coprocessor_enable_mask;

		static constexpr ui32 WRITE_MASK = 0b1111'0010'0111'1111'1111'1111'0011'1111;
	} sr;

	union CAUSE
	{
		ui32 bits;
		BitField<ui32, Exception, 2, 5> Excode; // which exception occurred
		BitField<ui32, ui8, 8, 8> Ip;            // interrupt pending
		BitField<ui32, ui8, 28, 2> CE;           // coprocessor number if caused by a coprocessor
		BitField<ui32, bool, 30, 1> BT;         // exception occurred in branch delay slot, and the branch was taken
		BitField<ui32, bool, 31, 1> BD;         // exception occurred in branch delay slot, but pushed IP is for branch

		static constexpr ui32 WRITE_MASK = 0b0000'0000'0000'0000'0000'0011'0000'0000;
	} cause;

	union DCIC
	{
		ui32 bits;
		BitField<ui32, bool, 0, 1> status_any_break;
		BitField<ui32, bool, 1, 1> status_bpc_code_break;
		BitField<ui32, bool, 2, 1> status_bda_data_break;
		BitField<ui32, bool, 3, 1> status_bda_data_read_break;
		BitField<ui32, bool, 4, 1> status_bda_data_write_break;
		BitField<ui32, bool, 5, 1> status_any_jump_break;
		BitField<ui32, ui8, 12, 2> jump_redirection;
		BitField<ui32, bool, 23, 1> super_master_enable_1;
		BitField<ui32, bool, 24, 1> execution_breakpoint_enable;
		BitField<ui32, bool, 25, 1> data_access_breakpoint;
		BitField<ui32, bool, 26, 1> break_on_data_read;
		BitField<ui32, bool, 27, 1> break_on_data_write;
		BitField<ui32, bool, 28, 1> break_on_any_jump;
		BitField<ui32, bool, 29, 1> master_enable_any_jump;
		BitField<ui32, bool, 30, 1> master_enable_break;
		BitField<ui32, bool, 31, 1> super_master_enable_2;

		static constexpr ui32 WRITE_MASK = 0b1111'1111'1000'0000'1111'0000'0011'1111;
	} dcic;
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

	void FlushPipeline();

	template<MemoryAccessType type, class T>
	void ProcessDispatch( ui32 address, std::conditional_t<type == MemoryAccessType::Read, ui32&, ui32> data );

	template<MemoryAccessType type, class T>
	void ProcessDispatchKUSEG( ui32 address, std::conditional_t<type == MemoryAccessType::Read, ui32&, ui32> data );

	template<MemoryAccessType type, class T>
	void ProcessBIOSDispatch( ui32 address, std::conditional_t<type == MemoryAccessType::Read, ui32&, ui32> data );

	template<MemoryAccessType type, class T>
	void ProcessRam( ui32 address, std::conditional_t<type == MemoryAccessType::Read, ui32&, ui32> data );

	template<class T>
	bool AlignmentCheck( ui32 address );

	template<MemoryAccessType type, class T>
	bool AlignmentCheck( ui32 address );

	bool InMemoryRange( ui32 start, ui32 end, ui32 data );

	void WriteRegister( ui8 writeRegister, ui32 value );

	ui32 ReadRegister( ui8 readRegister );

	void WriteMemoryWord( ui32 address, ui32 value );

	bool ReadMemoryWord( ui32 address, ui32& value );

	bool WriteMemoryHalfWord( ui32 address, ui16 value );

	bool WriteMemoryByte( ui32 address, ui8 value );

	void TakeBranch( ui32 address );

	void ExicuteCop0Instruction();

	void WriteCop0Reg( Cop0Reg reg, ui32 value );

	void RaiseException( Exception ex );

	void RaiseException( Exception ex, ui32 cPC, bool inBranchDelaySlot, bool currentInstructionBranchTaken, ui8 ce );

	bool AddOverflow( ui32 old, ui32 toAdd, ui32 add );

	void WriteRegDelayed( Register rd, ui32 value );

	void PreformLoadDelay();

	Registers mRegisters;

	Cop0Registers mCOP0Registers;

	Instruction mNextInstruction;

	Instruction mCurrentInstruction;


	bool mExceptionThrown;

	bool mCurrentInstructionIsBranchDelaySlot;
	bool mNextInstructionIsBranchDelaySlot;
	bool mCurrentBranchTaken;
	bool mNextBranchTaken;

	ui32 mCurrentInstructionPc;

	Register mNextLoadDelayRegister = Register::MAX;

	Register mLoadDelayRegister = Register::MAX;
	ui32 mNextLoadDelayValue = 0;

	ui32 mLoadDelayValue = 0;


	// PSX Memory Map
	// KUSEG      KSEG0      KSEG1      Length Description
	// 0x00000000 0x80000000 0xa0000000 2048K Main RAM
	// 0x1f000000 0x9f000000 0xbf000000 8192K Expansion Region 1
	// 0x1f800000 0x9f800000 0xbf800000 1K Scratchpad
	// 0x1f801000 0x9f801000 0xbf801000 8K Hardware registers
	// 0x1fc00000 0x9fc00000 0xbfc00000 512K BIOS ROM
	std::unique_ptr<ui8[]> mBIOS;


	std::unique_ptr<ui8[]> mRam;


	const unsigned int BIOS_START = 0x1FC00000;
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

	static const unsigned int CACHECTRL_START = 0xFFFE0130;
	static const unsigned int CACHECTRL_SIZE = 0x4;

	static constexpr ui32 DCACHE_LOCATION = UINT32_C( 0x1F800000 );
	static constexpr ui32 DCACHE_LOCATION_MASK = UINT32_C( 0xFFFFFC00 );
};




template<MemoryAccessType type, class T>
inline void EmuPSX::ProcessDispatch( ui32 address, std::conditional_t<type == MemoryAccessType::Read, ui32&, ui32> data )
{
	ui8 header = address >> 29;


	switch (header)
	{
		// kuseg
		case 0x00:
		{
			if constexpr (type == MemoryAccessType::Write)
			{
				// If the isolate cache flag is set, then all load and store operations are targetted
				// to the Data cache, and never the main memory.
				if (mCOP0Registers.sr.Isc)
				{
					return;
				}
			}

			const ui32 physicalAddress = address & 0x1FFFFFFF;


			if ((physicalAddress & DCACHE_LOCATION_MASK) == DCACHE_LOCATION)
			{
				//DoScratchpadAccess<type, T>( physicalAddress, data );
				return;
			}

			ProcessDispatchKUSEG<type, T>( physicalAddress, data );
			break;
		}
		// kseg0 or physical memory thats cached
		case 0x04:
		{
			if constexpr (type == MemoryAccessType::Write)
			{
				// If the isolate cache flag is set, then all load and store operations are targetted
				// to the Data cache, and never the main memory.
				if (mCOP0Registers.sr.Isc)
				{
					return;
				}
			}

			const ui32 physicalAddress = address & 0x1FFFFFFF;

			if ((physicalAddress & DCACHE_LOCATION_MASK) == DCACHE_LOCATION)
			{
				//DoScratchpadAccess<type, T>( physicalAddress, data );
				return;
			}
			ProcessDispatchKUSEG<type, T>( physicalAddress, data );
			break;
		}
		// kseg1 or physical memory thats uncached
		case 0x05:
		{
			const ui32 physicalAddress = address & 0x1FFFFFFF;
			ProcessDispatchKUSEG<type, T>( physicalAddress, data );
			break;
		}
		case 0x07:
		{
			if (address == 0xFFFE0130)
			{
				// To do later
			}
			break;
		}
			
		default:
		{
			std::cout << "Uninplemented KUSEG Range 0x" << std::hex << (int)header << std::endl;
			throw("Uninplemented");
			break;
		}
	}






}

template<MemoryAccessType type, class T>
inline void EmuPSX::ProcessDispatchKUSEG( ui32 address, std::conditional_t<type == MemoryAccessType::Read, ui32&, ui32> data )
{
	static constexpr int dataSize = sizeof( T );
	if (address < TotalRamSize)
	{
		ProcessRam<type, T>( address, data );
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
		// Ignore sound for now

	}
	else if (address < EXP2_BASE)
	{
		std::cout << "Uninplemented Memory 0x" << std::hex << address << std::endl;
		throw("Uninplemented");
	}
	else if (address < (EXP2_BASE + EXP2_SIZE))
	{
		// Only seemed to be used for PSX development
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

template<MemoryAccessType type, class T>
inline void EmuPSX::ProcessRam( ui32 address, std::conditional_t<type == MemoryAccessType::Read, ui32&, ui32> data )
{

	address &= 0x1FFFFF;

	static constexpr int dataSize = sizeof( T );

	if constexpr (type == MemoryAccessType::Read)
	{

		if constexpr (dataSize == 1) // Reading ui8
		{
			data = ZeroExtend32( mRam[address] );
		}
		else if constexpr (dataSize == 2) // Reading 16
		{
			ui16 tempData = *reinterpret_cast<ui16*>(&mRam[address]);

			data = ZeroExtend32( tempData );
		}
		else // 32 bit
		{
			data = *reinterpret_cast<ui32*>(&mRam[address]);
		}
	}
	else if constexpr (type == MemoryAccessType::Write)
	{
		if constexpr (dataSize == 1) // Reading ui8
		{
			mRam[address] = Truncate8( data );
		}
		else if constexpr (dataSize == 2) // Reading 16
		{
			ui16 tempData = Truncate16( data );
			*(reinterpret_cast<ui16*>(&mRam[address])) = tempData;
		}
		else // 32 bit
		{
			*(reinterpret_cast<ui32*>(&mRam[address])) = data;
		}
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