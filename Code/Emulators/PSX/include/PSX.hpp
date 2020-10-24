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

	function = 0,
	b        = 1,
	j        = 2,
	jal      = 3,
	beq      = 4,
	bne      = 5,
	blez     = 6,
	bgtz     = 7,
	addi     = 8,
	addiu    = 9,
	slti     = 10,
	sltiu    = 11,
	andi     = 12,
	ori      = 13,
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
	sw       = 43,
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

	Instruction GetNextInstruction();

	void ExecuteInstruction( Instruction instruction );

	ui32 ReadBus( ui32 address );

	bool InMemoryRange( ui32 start, ui32 end, ui32 data );

	void WriteRegister( ui8 writeRegister, ui32 value );

	Registers mRegisters;




	// PSX Memory Map
	// KUSEG      KSEG0      KSEG1      Length Description
	// 0x00000000 0x80000000 0xa0000000 2048K Main RAM
	// 0x1f000000 0x9f000000 0xbf000000 8192K Expansion Region 1
	// 0x1f800000 0x9f800000 0xbf800000 1K Scratchpad
	// 0x1f801000 0x9f801000 0xbf801000 8K Hardware registers
	// 0x1fc00000 0x9fc00000 0xbfc00000 512K BIOS ROM
	std::unique_ptr<ui8[]> mBIOS;

};