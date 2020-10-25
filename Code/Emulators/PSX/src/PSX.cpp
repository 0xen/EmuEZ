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

	mNextInstruction.data = 0x0;
	// Reset Registers
	GetRegister( Register::npc ) = 0xBFC00000;

	return false;
}

void EmuPSX::TickEmu()
{
	while (true) // Dont have a escape yet
	{
		GetNextInstruction();

		ExecuteInstruction();
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

void EmuPSX::GetNextInstruction()
{
	mExceptionThrown = false;

	mCurrentInstruction = mNextInstruction;

	mCurrentInstructionIsBranchDelaySlot = mNextInstructionIsBranchDelaySlot;
	mCurrentBranchTaken = mNextBranchTaken;

	mNextBranchTaken = false;
	mNextInstructionIsBranchDelaySlot = false;

	if (!AlignmentCheck<ui32>( mRegisters.pc ))
	{
		RaiseException( Exception::IBE, mRegisters.npc, false, false, 0 );
		return;
	}

	ProcessDispatch<MemoryAccessType::Read, ui32>( mRegisters.npc, mNextInstruction.data );

	mRegisters.pc = mRegisters.npc;

	mRegisters.npc += 4;
}

void EmuPSX::ExecuteInstruction()
{
	Instruction& instruction = mCurrentInstruction;

	std::cout << (int)instruction.op << std::endl;


	switch (instruction.op)
	{
		case InstructionOp::function: // 0: Function
		{
			switch (instruction.r.function)
			{
				case InstructionFunction::sll: // 0: Shift Left Logical
				{
					const ui32 value = ReadRegister( instruction.r.rt ) << instruction.r.shamt;
					WriteRegister( instruction.r.rd, value );
					break;
				}
				case InstructionFunction::or_: // 37: OR
				{
					const ui32 value = ReadRegister( instruction.r.rs ) | ReadRegister( instruction.r.rt );
					WriteRegister( instruction.r.rd, value );
					break;
				}
				default:
				{
					std::cout << "Invalid CPU Function " << (int)instruction.r.function << std::endl;
					throw("Unhandled function");
					break;
				}
			}
			break;
		}
		case InstructionOp::j: // 2: Jump
		{
			mNextInstructionIsBranchDelaySlot = true;
			TakeBranch( (mRegisters.pc & 0xF0000000) | (mCurrentInstruction.j.target << 2) );
			break;
		}
		case InstructionOp::addiu: // 9: Add Immediate Unsigned
		{
			WriteRegister( instruction.i.rt, ReadRegister( instruction.i.rs ) + instruction.i.immAsI32() );
			break;
		}
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
			std::cout << "Invalid CPU Instruction " << (int)instruction.op << " " << std::hex << instruction.data << std::endl;
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

void EmuPSX::TakeBranch( ui32 address )
{
	// Make sure the memory is in 4 bite alignment
	if (!AlignmentCheck<ui32>( address ))
	{
		RaiseException( Exception::AdEL, address, false, false, 0 );
		return;	
	}
	mRegisters.npc = address;
	mNextBranchTaken = true;
}

void EmuPSX::RaiseException( Exception ex )
{
	// To do
}

void EmuPSX::RaiseException( Exception ex, ui32 cPC, bool inBranchDelaySlot, bool currentInstructionBranchTaken, ui8 ce )
{
	// To do
}
