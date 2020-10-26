#include <PSX.hpp>
#include <IO.hpp>

#include <iostream>
#include <assert.h>

EmuPSX::EmuPSX()
{
}

bool EmuPSX::InitEmu( const char* path )
{
	path;

	LoadBIOS();

	mRam = std::unique_ptr<ui8[]>( new ui8[TotalRamSize] );

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

		PreformLoadDelay();
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
	mCurrentInstructionPc = mRegisters.pc;

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
				case InstructionFunction::jr: // 8: Jump register
				{
					const ui32 target = ReadRegister( instruction.r.rs );
					mNextInstructionIsBranchDelaySlot = true;
					TakeBranch( target );
					break;
				}
				case InstructionFunction::addu: // 33: Add Unsigned
				{
					const ui32 new_value = ReadRegister( instruction.r.rs ) + ReadRegister( instruction.r.rt );
					WriteRegister( instruction.r.rd, new_value );
					break;
				}
				case InstructionFunction::or_: // 37: OR
				{
					const ui32 value = ReadRegister( instruction.r.rs ) | ReadRegister( instruction.r.rt );
					WriteRegister( instruction.r.rd, value );
					break;
				}
				case InstructionFunction::sltu: // 43: Set on less than unsigned
				{
					const ui32 result = static_cast<ui32>(ReadRegister( instruction.r.rs ) < ReadRegister( instruction.r.rt ));
					WriteRegister( instruction.r.rd, result );
					break;
				}
				default:
				{
					std::cout << "Invalid CPU Function " << (int)instruction.r.function << " " << std::hex << instruction.data << std::endl;
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
		case InstructionOp::jal: // 3: Jump and link
		{
			mNextInstructionIsBranchDelaySlot = true;
			WriteRegister( Register::ra, mRegisters.npc );
			TakeBranch( (mRegisters.pc & 0xF0000000) | (mCurrentInstruction.j.target << 2) );
			break;
		}
		case InstructionOp::bne: // 5: Branch not equals
		{
			mNextInstructionIsBranchDelaySlot = true;
			const bool branch = (ReadRegister( instruction.i.rs ) != ReadRegister( instruction.i.rt ));
			if (branch)
				TakeBranch( mRegisters.pc + (instruction.i.immAsI32() << 2) );
			break;
		}
		case InstructionOp::addi: // 8: Add Immediate Signed
		{
			const ui32 old_value = ReadRegister( instruction.r.rs );
			const ui32 add_value = instruction.i.immAsI32();
			const ui32 new_value = old_value + add_value;

			if (AddOverflow( old_value, add_value, new_value ))
			{
				RaiseException( Exception::Ov );
				return;
			}
			WriteRegister( instruction.r.rt, new_value );
			break;
		}
		case InstructionOp::addiu: // 9: Add Immediate Unsigned
		{
			WriteRegister( instruction.i.rt, ReadRegister( instruction.i.rs ) + instruction.i.immAsI32() );
			break;
		}
		case InstructionOp::andi: // 12: Bitwise and immediate
		{
			WriteRegister( instruction.i.rt, ReadRegister( instruction.i.rs ) & instruction.i.immAsUI32() );
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
		case InstructionOp::cop0: // 16: Co Processor 0
		{
			ExicuteCop0Instruction();
			break;
		}
		case InstructionOp::lw: // 35: Load Word
		{
			// Skip if cache is isolated
			if (mCOP0Registers.sr.Isc)
			{
				return;
			}

			ui32 address = ReadRegister( instruction.i.rs ) + instruction.i.immAsI32();

			ui32 value;
			if (!ReadMemoryWord( address, value ))
			{
				return;
			}

			WriteRegDelayed( instruction.i.rt, value );
			break;
		}
		case InstructionOp::sb: // 40: Store byte
		{
			const ui32 addr = ReadRegister( instruction.i.rs ) + instruction.i.immAsI32();
			const ui8 value = static_cast<ui8>(ReadRegister( instruction.i.rt ));
			WriteMemoryByte( addr, value );
			break;
		}
		case InstructionOp::sh: // 41: Store halfword
		{
			const ui32 addr = ReadRegister( instruction.i.rs ) + instruction.i.immAsI32();
			const ui16 value = static_cast<ui16>(ReadRegister( instruction.i.rt ));
			WriteMemoryHalfWord( addr, value );
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

void EmuPSX::FlushPipeline()
{
	mNextLoadDelayRegister = Register::MAX;

	if (mLoadDelayRegister != Register::MAX)
	{
		mRegisters.data[static_cast<ui8>(mLoadDelayRegister)] = mLoadDelayValue;
	}


	GetNextInstruction();

	mNextBranchTaken = false;
	mNextInstructionIsBranchDelaySlot = false;
}

bool EmuPSX::InMemoryRange( ui32 start, ui32 end, ui32 data )
{
	return start <= data && end >= data;
}

void EmuPSX::WriteRegister( ui8 writeRegister, ui32 value )
{
	mRegisters.data[writeRegister] = value;

	// If the write register is also the load delay, reset it
	mLoadDelayRegister = (writeRegister == mLoadDelayRegister) ? Register::MAX : mLoadDelayRegister;

	// Make sure zero is never set
	mRegisters.data[0] = 0;
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

bool EmuPSX::ReadMemoryWord( ui32 address, ui32& value )
{
	// Make sure the memory is in 4 bite alignment
	if (!AlignmentCheck<MemoryAccessType::Read, ui32>( address ))
	{
		std::cout << "Memory Not Alligned" << std::endl;
		return false;
	}

	ProcessDispatch<MemoryAccessType::Read, ui32>( address, value );

	return true;
}

bool EmuPSX::WriteMemoryHalfWord( ui32 address, ui16 value )
{
	// Make sure the memory is in 2 bite alignment
	if (!AlignmentCheck<MemoryAccessType::Write, ui16>( address ))
	{
		std::cout << "Memory Not Alligned" << std::endl;
		return false;
	}
	ui32 temp = ZeroExtend32( value );
	ProcessDispatch<MemoryAccessType::Write, ui16>( address, temp );

	// Need to process Write cycle count

	return true;
}

bool EmuPSX::WriteMemoryByte( ui32 address, ui8 value )
{
	ui32 temp = ZeroExtend32( value );
	ProcessDispatch<MemoryAccessType::Write, ui8>( address, temp );

	// Need to process Write cycle count

	return true;
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

void EmuPSX::ExicuteCop0Instruction()
{
	Instruction& instruction = mCurrentInstruction;



	if (instruction.cop.IsCommonInstruction())
	{

		switch (instruction.cop.CommonOp())
		{

			// Move to co-processor 'n'
			case CopCommonInstruction::mtcn:
			{
				WriteCop0Reg( static_cast<Cop0Reg>(instruction.r.rd.Get()), ReadRegister( instruction.r.rt ) );
				break;
			}
			default:
			{
				std::cout << "Invalid COP0 Instruction " << (int)instruction.cop.CommonOp() << std::endl;
				throw("Unhandled instruction");
				break;
			}
		}
	}
	else
	{
		throw("uninplemented");
	}
}

void EmuPSX::WriteCop0Reg( Cop0Reg reg, ui32 value )
{
	switch (reg)
	{
		case Cop0Reg::BPC: // 3: Break point Exception
		{
			mCOP0Registers.BPC = value;
			break;
		}
		case Cop0Reg::BDA: // 5: Not sure
		{
			mCOP0Registers.BDA = value;
			break;
		}
		case Cop0Reg::JUMPDEST:
		{
			break;
		}
		case Cop0Reg::DCIC: // 7: Enable / Dissable heardware break points
		{
			mCOP0Registers.dcic.bits =
				(mCOP0Registers.dcic.bits & ~Cop0Registers::DCIC::WRITE_MASK) | (value & Cop0Registers::DCIC::WRITE_MASK);
			break;
		}
		case Cop0Reg::BDAM: // 9: Bitmask applied to BDA
		{
			mCOP0Registers.BDAM = value;
			break;
		}
		case Cop0Reg::BPCM: // 11: Data Break point
		{
			mCOP0Registers.BPCM = value;
			break;
		}
		case Cop0Reg::SR: // 12: Status Register
		{
			mCOP0Registers.sr.bits =
				(mCOP0Registers.sr.bits & ~Cop0Registers::SR::WRITE_MASK) | (value & Cop0Registers::SR::WRITE_MASK);
			break;
		}
		case Cop0Reg::CAUSE: // 13: Read only data describing an exception
		{
			mCOP0Registers.cause.bits =
				(mCOP0Registers.cause.bits & ~Cop0Registers::CAUSE::WRITE_MASK) | (value & Cop0Registers::CAUSE::WRITE_MASK);
			break;
		}
		default:
		{
			std::cout << "Invalid COP0 Register " << (int)reg << std::endl;
			throw("Unhandled instruction");
			break;
		}
	}

}

void EmuPSX::RaiseException( Exception ex )
{
	RaiseException( ex, mCurrentInstructionPc, mNextInstructionIsBranchDelaySlot, mNextBranchTaken, mCurrentInstruction.cop.cop_n );
}

void EmuPSX::RaiseException( Exception ex, ui32 cPC, bool inBranchDelaySlot, bool currentInstructionBranchTaken, ui8 ce )
{

	mCOP0Registers.EPC = cPC;
	mCOP0Registers.cause.Excode = ex;
	mCOP0Registers.cause.BD = inBranchDelaySlot;
	mCOP0Registers.cause.BT = currentInstructionBranchTaken;
	mCOP0Registers.cause.CE = ce;

	// TAR is set to the address which was being fetched in this instruction, or the next instruction to execute if the
	// exception hadn't occurred in the delay slot.
	if (inBranchDelaySlot)
	{
		mCOP0Registers.EPC -= UINT32_C( 4 );
		mCOP0Registers.TAR = mRegisters.pc;
	}


	// current -> previous, switch to kernel mode and disable interrupts
	mCOP0Registers.sr.mode_bits <<= 2;

	{
		const ui32 base = mCOP0Registers.sr.BEV ? UINT32_C( 0xbfc00100 ) : UINT32_C( 0x80000000 );
		mRegisters.npc = base | UINT32_C( 0x00000080 );
	}
	mExceptionThrown = true;
	// Flush the pipeline as we do not want to exicute the bad instruciton
	FlushPipeline();
}

bool EmuPSX::AddOverflow( ui32 old, ui32 toAdd, ui32 add )
{
	return (((add ^ old) & (add ^ toAdd)) & UINT32_C( 0x80000000 )) != 0;
}

void EmuPSX::WriteRegDelayed( Register rd, ui32 value )
{
	// Throw a debug error if the register is out of range
	assert( mNextLoadDelayRegister == Register::MAX && "Invalid next register" );

	if (rd == Register::zero)
	{
		return;
	}

	// If after this instruction we are supposed to set this register, forget about it and offset it to the next one
	if (mLoadDelayRegister == rd)
	{
		mLoadDelayRegister = Register::MAX;
	}

	mNextLoadDelayRegister = rd;
	mNextLoadDelayValue = value;
}

void EmuPSX::PreformLoadDelay()
{
	if (mLoadDelayRegister != Register::MAX)
	{
		mRegisters.data[static_cast<ui8>(mLoadDelayRegister)] = mLoadDelayValue;
	}

	mLoadDelayRegister = mNextLoadDelayRegister;
	mLoadDelayValue = mNextLoadDelayValue;
	mNextLoadDelayRegister = Register::MAX;
}
