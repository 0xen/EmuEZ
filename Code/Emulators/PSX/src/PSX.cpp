#include <PSX.hpp>
#include <IO.hpp>

#include <iostream>

EmuPSX::EmuPSX()
{
}

bool EmuPSX::InitEmu( const char* path )
{
	path;

	LoadBIOS();

	for (int i = 0; i < Register::MAX; i++)
		mRegisters.data[i] = 0;

	// Reset Registers
	GetRegister( Register::pc ) = 0xBFC00000;

	return false;
}

void EmuPSX::TickEmu()
{
	while (true) // Dont have a escape yet
	{
		Instruction instruction = GetNextInstruction();

		ExecuteInstruction( instruction );
	}
}

void EmuPSX::KeyPressEmu( ConsoleKeys key )
{
	key;
}

void EmuPSX::KeyReleaseEmu( ConsoleKeys key )
{
	key;
}

bool EmuPSX::IsKeyDownEmu( ConsoleKeys key )
{
	key;
	return false;
}

unsigned int EmuPSX::ScreenWidthEmu()
{
	return 0;
}

unsigned int EmuPSX::ScreenHeightEmu()
{
	return 0;
}

void EmuPSX::GetScreenBufferEmu( char*& ptr, unsigned int& size )
{
	ptr = nullptr;
	size = 0;
}

ui32& EmuPSX::GetRegister( Register reg  )
{
	return mRegisters.data[static_cast<int>(reg)];
}

void EmuPSX::LoadBIOS()
{
	const unsigned int origionalBIOSSize = 1024 * 512;
	unsigned int biosSize;
	mBIOS = std::unique_ptr<ui8[]>( LoadBinary( "BIOS/SCPH1001.bin", biosSize ) );

	if (origionalBIOSSize != biosSize)
	{
		throw("BIOS is wrong size");
	}
}

Instruction EmuPSX::GetNextInstruction()
{
	ui32 pc = GetRegister( Register::pc );

	GetRegister( Register::pc ) += 4;

	Instruction instruction;

	ProcessDispatch<MemoryAccessType::Read, ui32>( pc, instruction.data );

	return instruction;
}

void EmuPSX::ExecuteInstruction( Instruction instruction )
{


	std::cout << (int)instruction.op << std::endl;


	switch (instruction.op)
	{
		case InstructionOp::ori: // 13: Or Immidiate
		{
			WriteRegister( instruction.i.rt, ReadRegister( instruction.i.rs ) | instruction.i.immAsUI32() );
			break;
		}
		case InstructionOp::lui: // 15: Load upper immidiate
		{
			WriteRegister( instruction.i.rt, instruction.i.immAsUI32() << 16 );
			break;
		}
		case InstructionOp::sw: // 43: Store Word
		{
			const ui32 address = ReadRegister( instruction.i.rs ) + instruction.i.immAsI32();
			const ui32 value = ReadRegister( instruction.i.rt );

			WriteMemoryWord( address, value );
			break;
		}
		default:
		{
			std::cout << "Invalid CPU Instruction " << (int)instruction.op << std::endl;
			throw("Unhandled instruction");
			break;
		}
	}




}

bool EmuPSX::InMemoryRange( ui32 start, ui32 end, ui32 data )
{
	return start <= data && end >= data;
}

void EmuPSX::WriteRegister( ui8 writeRegister, ui32 value )
{
	mRegisters.data[writeRegister] = value;
}

ui32 EmuPSX::ReadRegister( ui8 readRegister )
{
	return mRegisters.data[readRegister];
}

void EmuPSX::WriteMemoryWord( ui32 address, ui32 value )
{
	if (!AlignmentCheck<MemoryAccessType::Write, ui32>( address ))
	{
		throw( "Unaligned Memory" );
	}

	// Need to do bounds check
	ProcessDispatch<MemoryAccessType::Write, ui32>( address, value );
}