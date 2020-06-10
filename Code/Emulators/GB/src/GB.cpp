#include "..\include\GB.hpp"
#include "../../../Util/include/IO.hpp"

#include <iostream>
#include <assert.h>


EmuGB::EmuGB()
{
	InitOPJumpTables();
}

bool EmuGB::InitEmu(const char* path)
{
	bool loaded = m_cartridge.Load(path);
	if (!loaded)return false;

	Reset();

	return true;
}

void EmuGB::TickEmu()
{
	std::cout << "Ticking GB" << std::endl;

	// Temp
	while (true)
	{
		m_op_code = ReadByteFromPC();

		m_cycle = (m_cycles[m_op_code] * GetCycleModifier(4));

		m_frame_cycles += m_cycle;

		if (m_op_code == 0xCB)
		{
			m_op_code = ReadByteFromPC();

			m_cycle = (m_cycles[256 + m_op_code] * GetCycleModifier(4));

			m_frame_cycles += m_cycle;

			(this->*m_CBOpCodes[m_op_code])();
		}
		else
		{
			(this->*m_opCodes[m_op_code])();
		}
	}

}

void EmuGB::InitOPJumpTables()
{
	m_opCodes[0x00] = &EmuGB::Op00;
	m_opCodes[0x01] = &EmuGB::Op01;
	m_opCodes[0x02] = &EmuGB::Op02;
	m_opCodes[0x03] = &EmuGB::Op03;
	m_opCodes[0x04] = &EmuGB::Op04;
	m_opCodes[0x05] = &EmuGB::Op05;
	m_opCodes[0x06] = &EmuGB::Op06;
	m_opCodes[0x07] = &EmuGB::Op07;
	m_opCodes[0x08] = &EmuGB::Op08;
	m_opCodes[0x09] = &EmuGB::Op09;
	m_opCodes[0x0A] = &EmuGB::Op0A;
	m_opCodes[0x0B] = &EmuGB::Op0B;
	m_opCodes[0x0C] = &EmuGB::Op0C;
	m_opCodes[0x0D] = &EmuGB::Op0D;
	m_opCodes[0x0E] = &EmuGB::Op0E;
	m_opCodes[0x0F] = &EmuGB::Op0F;

	m_opCodes[0x10] = &EmuGB::Op10;
	m_opCodes[0x11] = &EmuGB::Op11;
	m_opCodes[0x12] = &EmuGB::Op12;
	m_opCodes[0x13] = &EmuGB::Op13;
	m_opCodes[0x14] = &EmuGB::Op14;
	m_opCodes[0x15] = &EmuGB::Op15;
	m_opCodes[0x16] = &EmuGB::Op16;
	m_opCodes[0x17] = &EmuGB::Op17;
	m_opCodes[0x18] = &EmuGB::Op18;
	m_opCodes[0x19] = &EmuGB::Op19;
	m_opCodes[0x1A] = &EmuGB::Op1A;
	m_opCodes[0x1B] = &EmuGB::Op1B;
	m_opCodes[0x1C] = &EmuGB::Op1C;
	m_opCodes[0x1D] = &EmuGB::Op1D;
	m_opCodes[0x1E] = &EmuGB::Op1E;
	m_opCodes[0x1F] = &EmuGB::Op1F;

	m_opCodes[0x20] = &EmuGB::Op20;
	m_opCodes[0x21] = &EmuGB::Op21;
	m_opCodes[0x22] = &EmuGB::Op22;
	m_opCodes[0x23] = &EmuGB::Op23;
	m_opCodes[0x24] = &EmuGB::Op24;
	m_opCodes[0x25] = &EmuGB::Op25;
	m_opCodes[0x26] = &EmuGB::Op26;
	m_opCodes[0x27] = &EmuGB::Op27;
	m_opCodes[0x28] = &EmuGB::Op28;
	m_opCodes[0x29] = &EmuGB::Op29;
	m_opCodes[0x2A] = &EmuGB::Op2A;
	m_opCodes[0x2B] = &EmuGB::Op2B;
	m_opCodes[0x2C] = &EmuGB::Op2C;
	m_opCodes[0x2D] = &EmuGB::Op2D;
	m_opCodes[0x2E] = &EmuGB::Op2E;
	m_opCodes[0x2F] = &EmuGB::Op2F;

	m_opCodes[0x30] = &EmuGB::Op30;
	m_opCodes[0x31] = &EmuGB::Op31;
	m_opCodes[0x32] = &EmuGB::Op32;
	m_opCodes[0x33] = &EmuGB::Op33;
	m_opCodes[0x34] = &EmuGB::Op34;
	m_opCodes[0x35] = &EmuGB::Op35;
	m_opCodes[0x36] = &EmuGB::Op36;
	m_opCodes[0x37] = &EmuGB::Op37;
	m_opCodes[0x38] = &EmuGB::Op38;
	m_opCodes[0x39] = &EmuGB::Op39;
	m_opCodes[0x3A] = &EmuGB::Op3A;
	m_opCodes[0x3B] = &EmuGB::Op3B;
	m_opCodes[0x3C] = &EmuGB::Op3C;
	m_opCodes[0x3D] = &EmuGB::Op3D;
	m_opCodes[0x3E] = &EmuGB::Op3E;
	m_opCodes[0x3F] = &EmuGB::Op3F;

	m_opCodes[0x40] = &EmuGB::Op40;
	m_opCodes[0x41] = &EmuGB::Op41;
	m_opCodes[0x42] = &EmuGB::Op42;
	m_opCodes[0x43] = &EmuGB::Op43;
	m_opCodes[0x44] = &EmuGB::Op44;
	m_opCodes[0x45] = &EmuGB::Op45;
	m_opCodes[0x46] = &EmuGB::Op46;
	m_opCodes[0x47] = &EmuGB::Op47;
	m_opCodes[0x48] = &EmuGB::Op48;
	m_opCodes[0x49] = &EmuGB::Op49;
	m_opCodes[0x4A] = &EmuGB::Op4A;
	m_opCodes[0x4B] = &EmuGB::Op4B;
	m_opCodes[0x4C] = &EmuGB::Op4C;
	m_opCodes[0x4D] = &EmuGB::Op4D;
	m_opCodes[0x4E] = &EmuGB::Op4E;
	m_opCodes[0x4F] = &EmuGB::Op4F;

	m_opCodes[0x50] = &EmuGB::Op50;
	m_opCodes[0x51] = &EmuGB::Op51;
	m_opCodes[0x52] = &EmuGB::Op52;
	m_opCodes[0x53] = &EmuGB::Op53;
	m_opCodes[0x54] = &EmuGB::Op54;
	m_opCodes[0x55] = &EmuGB::Op55;
	m_opCodes[0x56] = &EmuGB::Op56;
	m_opCodes[0x57] = &EmuGB::Op57;
	m_opCodes[0x58] = &EmuGB::Op58;
	m_opCodes[0x59] = &EmuGB::Op59;
	m_opCodes[0x5A] = &EmuGB::Op5A;
	m_opCodes[0x5B] = &EmuGB::Op5B;
	m_opCodes[0x5C] = &EmuGB::Op5C;
	m_opCodes[0x5D] = &EmuGB::Op5D;
	m_opCodes[0x5E] = &EmuGB::Op5E;
	m_opCodes[0x5F] = &EmuGB::Op5F;

	m_opCodes[0x60] = &EmuGB::Op60;
	m_opCodes[0x61] = &EmuGB::Op61;
	m_opCodes[0x62] = &EmuGB::Op62;
	m_opCodes[0x63] = &EmuGB::Op63;
	m_opCodes[0x64] = &EmuGB::Op64;
	m_opCodes[0x65] = &EmuGB::Op65;
	m_opCodes[0x66] = &EmuGB::Op66;
	m_opCodes[0x67] = &EmuGB::Op67;
	m_opCodes[0x68] = &EmuGB::Op68;
	m_opCodes[0x69] = &EmuGB::Op69;
	m_opCodes[0x6A] = &EmuGB::Op6A;
	m_opCodes[0x6B] = &EmuGB::Op6B;
	m_opCodes[0x6C] = &EmuGB::Op6C;
	m_opCodes[0x6D] = &EmuGB::Op6D;
	m_opCodes[0x6E] = &EmuGB::Op6E;
	m_opCodes[0x6F] = &EmuGB::Op6F;

	m_opCodes[0x70] = &EmuGB::Op70;
	m_opCodes[0x71] = &EmuGB::Op71;
	m_opCodes[0x72] = &EmuGB::Op72;
	m_opCodes[0x73] = &EmuGB::Op73;
	m_opCodes[0x74] = &EmuGB::Op74;
	m_opCodes[0x75] = &EmuGB::Op75;
	m_opCodes[0x76] = &EmuGB::Op76;
	m_opCodes[0x77] = &EmuGB::Op77;
	m_opCodes[0x78] = &EmuGB::Op78;
	m_opCodes[0x79] = &EmuGB::Op79;
	m_opCodes[0x7A] = &EmuGB::Op7A;
	m_opCodes[0x7B] = &EmuGB::Op7B;
	m_opCodes[0x7C] = &EmuGB::Op7C;
	m_opCodes[0x7D] = &EmuGB::Op7D;
	m_opCodes[0x7E] = &EmuGB::Op7E;
	m_opCodes[0x7F] = &EmuGB::Op7F;

	m_opCodes[0x80] = &EmuGB::Op80;
	m_opCodes[0x81] = &EmuGB::Op81;
	m_opCodes[0x82] = &EmuGB::Op82;
	m_opCodes[0x83] = &EmuGB::Op83;
	m_opCodes[0x84] = &EmuGB::Op84;
	m_opCodes[0x85] = &EmuGB::Op85;
	m_opCodes[0x86] = &EmuGB::Op86;
	m_opCodes[0x87] = &EmuGB::Op87;
	m_opCodes[0x88] = &EmuGB::Op88;
	m_opCodes[0x89] = &EmuGB::Op89;
	m_opCodes[0x8A] = &EmuGB::Op8A;
	m_opCodes[0x8B] = &EmuGB::Op8B;
	m_opCodes[0x8C] = &EmuGB::Op8C;
	m_opCodes[0x8D] = &EmuGB::Op8D;
	m_opCodes[0x8E] = &EmuGB::Op8E;
	m_opCodes[0x8F] = &EmuGB::Op8F;

	m_opCodes[0x90] = &EmuGB::Op90;
	m_opCodes[0x91] = &EmuGB::Op91;
	m_opCodes[0x92] = &EmuGB::Op92;
	m_opCodes[0x93] = &EmuGB::Op93;
	m_opCodes[0x94] = &EmuGB::Op94;
	m_opCodes[0x95] = &EmuGB::Op95;
	m_opCodes[0x96] = &EmuGB::Op96;
	m_opCodes[0x97] = &EmuGB::Op97;
	m_opCodes[0x98] = &EmuGB::Op98;
	m_opCodes[0x99] = &EmuGB::Op99;
	m_opCodes[0x9A] = &EmuGB::Op9A;
	m_opCodes[0x9B] = &EmuGB::Op9B;
	m_opCodes[0x9C] = &EmuGB::Op9C;
	m_opCodes[0x9D] = &EmuGB::Op9D;
	m_opCodes[0x9E] = &EmuGB::Op9E;
	m_opCodes[0x9F] = &EmuGB::Op9F;

	m_opCodes[0xA0] = &EmuGB::OpA0;
	m_opCodes[0xA1] = &EmuGB::OpA1;
	m_opCodes[0xA2] = &EmuGB::OpA2;
	m_opCodes[0xA3] = &EmuGB::OpA3;
	m_opCodes[0xA4] = &EmuGB::OpA4;
	m_opCodes[0xA5] = &EmuGB::OpA5;
	m_opCodes[0xA6] = &EmuGB::OpA6;
	m_opCodes[0xA7] = &EmuGB::OpA7;
	m_opCodes[0xA8] = &EmuGB::OpA8;
	m_opCodes[0xA9] = &EmuGB::OpA9;
	m_opCodes[0xAA] = &EmuGB::OpAA;
	m_opCodes[0xAB] = &EmuGB::OpAB;
	m_opCodes[0xAC] = &EmuGB::OpAC;
	m_opCodes[0xAD] = &EmuGB::OpAD;
	m_opCodes[0xAE] = &EmuGB::OpAE;
	m_opCodes[0xAF] = &EmuGB::OpAF;

	m_opCodes[0xB0] = &EmuGB::OpB0;
	m_opCodes[0xB1] = &EmuGB::OpB1;
	m_opCodes[0xB2] = &EmuGB::OpB2;
	m_opCodes[0xB3] = &EmuGB::OpB3;
	m_opCodes[0xB4] = &EmuGB::OpB4;
	m_opCodes[0xB5] = &EmuGB::OpB5;
	m_opCodes[0xB6] = &EmuGB::OpB6;
	m_opCodes[0xB7] = &EmuGB::OpB7;
	m_opCodes[0xB8] = &EmuGB::OpB8;
	m_opCodes[0xB9] = &EmuGB::OpB9;
	m_opCodes[0xBA] = &EmuGB::OpBA;
	m_opCodes[0xBB] = &EmuGB::OpBB;
	m_opCodes[0xBC] = &EmuGB::OpBC;
	m_opCodes[0xBD] = &EmuGB::OpBD;
	m_opCodes[0xBE] = &EmuGB::OpBE;
	m_opCodes[0xBF] = &EmuGB::OpBF;

	m_opCodes[0xC0] = &EmuGB::OpC0;
	m_opCodes[0xC1] = &EmuGB::OpC1;
	m_opCodes[0xC2] = &EmuGB::OpC2;
	m_opCodes[0xC3] = &EmuGB::OpC3;
	m_opCodes[0xC4] = &EmuGB::OpC4;
	m_opCodes[0xC5] = &EmuGB::OpC5;
	m_opCodes[0xC6] = &EmuGB::OpC6;
	m_opCodes[0xC7] = &EmuGB::OpC7;
	m_opCodes[0xC8] = &EmuGB::OpC8;
	m_opCodes[0xC9] = &EmuGB::OpC9;
	m_opCodes[0xCA] = &EmuGB::OpCA;
	//m_opCodes[0xCB] = &EmuGB::OpCB;
	m_opCodes[0xCC] = &EmuGB::OpCC;
	m_opCodes[0xCD] = &EmuGB::OpCD;
	m_opCodes[0xCE] = &EmuGB::OpCE;
	m_opCodes[0xCF] = &EmuGB::OpCF;

	m_opCodes[0xD0] = &EmuGB::OpD0;
	m_opCodes[0xD1] = &EmuGB::OpD1;
	m_opCodes[0xD2] = &EmuGB::OpD2;
	m_opCodes[0xD3] = &EmuGB::OpD3;
	m_opCodes[0xD4] = &EmuGB::OpD4;
	m_opCodes[0xD5] = &EmuGB::OpD5;
	m_opCodes[0xD6] = &EmuGB::OpD6;
	m_opCodes[0xD7] = &EmuGB::OpD7;
	m_opCodes[0xD8] = &EmuGB::OpD8;
	m_opCodes[0xD9] = &EmuGB::OpD9;
	m_opCodes[0xDA] = &EmuGB::OpDA;
	m_opCodes[0xDB] = &EmuGB::OpDB;
	m_opCodes[0xDC] = &EmuGB::OpDC;
	m_opCodes[0xDD] = &EmuGB::OpDD;
	m_opCodes[0xDE] = &EmuGB::OpDE;
	m_opCodes[0xDF] = &EmuGB::OpDF;

	m_opCodes[0xE0] = &EmuGB::OpE0;
	m_opCodes[0xE1] = &EmuGB::OpE1;
	m_opCodes[0xE2] = &EmuGB::OpE2;
	m_opCodes[0xE3] = &EmuGB::OpE3;
	m_opCodes[0xE4] = &EmuGB::OpE4;
	m_opCodes[0xE5] = &EmuGB::OpE5;
	m_opCodes[0xE6] = &EmuGB::OpE6;
	m_opCodes[0xE7] = &EmuGB::OpE7;
	m_opCodes[0xE8] = &EmuGB::OpE8;
	m_opCodes[0xE9] = &EmuGB::OpE9;
	m_opCodes[0xEA] = &EmuGB::OpEA;
	m_opCodes[0xEB] = &EmuGB::OpEB;
	m_opCodes[0xEC] = &EmuGB::OpEC;
	m_opCodes[0xED] = &EmuGB::OpED;
	m_opCodes[0xEE] = &EmuGB::OpEE;
	m_opCodes[0xEF] = &EmuGB::OpEF;

	m_opCodes[0xF0] = &EmuGB::OpF0;
	m_opCodes[0xF1] = &EmuGB::OpF1;
	m_opCodes[0xF2] = &EmuGB::OpF2;
	m_opCodes[0xF3] = &EmuGB::OpF3;
	m_opCodes[0xF4] = &EmuGB::OpF4;
	m_opCodes[0xF5] = &EmuGB::OpF5;
	m_opCodes[0xF6] = &EmuGB::OpF6;
	m_opCodes[0xF7] = &EmuGB::OpF7;
	m_opCodes[0xF8] = &EmuGB::OpF8;
	m_opCodes[0xF9] = &EmuGB::OpF9;
	m_opCodes[0xFA] = &EmuGB::OpFA;
	m_opCodes[0xFB] = &EmuGB::OpFB;
	m_opCodes[0xFC] = &EmuGB::OpFC;
	m_opCodes[0xFD] = &EmuGB::OpFD;
	m_opCodes[0xFE] = &EmuGB::OpFE;
	m_opCodes[0xFF] = &EmuGB::OpFF;


	m_CBOpCodes[0x00] = &EmuGB::OpCB00;
	m_CBOpCodes[0x01] = &EmuGB::OpCB01;
	m_CBOpCodes[0x02] = &EmuGB::OpCB02;
	m_CBOpCodes[0x03] = &EmuGB::OpCB03;
	m_CBOpCodes[0x04] = &EmuGB::OpCB04;
	m_CBOpCodes[0x05] = &EmuGB::OpCB05;
	m_CBOpCodes[0x06] = &EmuGB::OpCB06;
	m_CBOpCodes[0x07] = &EmuGB::OpCB07;
	m_CBOpCodes[0x08] = &EmuGB::OpCB08;
	m_CBOpCodes[0x09] = &EmuGB::OpCB09;
	m_CBOpCodes[0x0A] = &EmuGB::OpCB0A;
	m_CBOpCodes[0x0B] = &EmuGB::OpCB0B;
	m_CBOpCodes[0x0C] = &EmuGB::OpCB0C;
	m_CBOpCodes[0x0D] = &EmuGB::OpCB0D;
	m_CBOpCodes[0x0E] = &EmuGB::OpCB0E;
	m_CBOpCodes[0x0F] = &EmuGB::OpCB0F;

	m_CBOpCodes[0x10] = &EmuGB::OpCB10;
	m_CBOpCodes[0x11] = &EmuGB::OpCB11;
	m_CBOpCodes[0x12] = &EmuGB::OpCB12;
	m_CBOpCodes[0x13] = &EmuGB::OpCB13;
	m_CBOpCodes[0x14] = &EmuGB::OpCB14;
	m_CBOpCodes[0x15] = &EmuGB::OpCB15;
	m_CBOpCodes[0x16] = &EmuGB::OpCB16;
	m_CBOpCodes[0x17] = &EmuGB::OpCB17;
	m_CBOpCodes[0x18] = &EmuGB::OpCB18;
	m_CBOpCodes[0x19] = &EmuGB::OpCB19;
	m_CBOpCodes[0x1A] = &EmuGB::OpCB1A;
	m_CBOpCodes[0x1B] = &EmuGB::OpCB1B;
	m_CBOpCodes[0x1C] = &EmuGB::OpCB1C;
	m_CBOpCodes[0x1D] = &EmuGB::OpCB1D;
	m_CBOpCodes[0x1E] = &EmuGB::OpCB1E;
	m_CBOpCodes[0x1F] = &EmuGB::OpCB1F;

	m_CBOpCodes[0x20] = &EmuGB::OpCB20;
	m_CBOpCodes[0x21] = &EmuGB::OpCB21;
	m_CBOpCodes[0x22] = &EmuGB::OpCB22;
	m_CBOpCodes[0x23] = &EmuGB::OpCB23;
	m_CBOpCodes[0x24] = &EmuGB::OpCB24;
	m_CBOpCodes[0x25] = &EmuGB::OpCB25;
	m_CBOpCodes[0x26] = &EmuGB::OpCB26;
	m_CBOpCodes[0x27] = &EmuGB::OpCB27;
	m_CBOpCodes[0x28] = &EmuGB::OpCB28;
	m_CBOpCodes[0x29] = &EmuGB::OpCB29;
	m_CBOpCodes[0x2A] = &EmuGB::OpCB2A;
	m_CBOpCodes[0x2B] = &EmuGB::OpCB2B;
	m_CBOpCodes[0x2C] = &EmuGB::OpCB2C;
	m_CBOpCodes[0x2D] = &EmuGB::OpCB2D;
	m_CBOpCodes[0x2E] = &EmuGB::OpCB2E;
	m_CBOpCodes[0x2F] = &EmuGB::OpCB2F;

	m_CBOpCodes[0x30] = &EmuGB::OpCB30;
	m_CBOpCodes[0x31] = &EmuGB::OpCB31;
	m_CBOpCodes[0x32] = &EmuGB::OpCB32;
	m_CBOpCodes[0x33] = &EmuGB::OpCB33;
	m_CBOpCodes[0x34] = &EmuGB::OpCB34;
	m_CBOpCodes[0x35] = &EmuGB::OpCB35;
	m_CBOpCodes[0x36] = &EmuGB::OpCB36;
	m_CBOpCodes[0x37] = &EmuGB::OpCB37;
	m_CBOpCodes[0x38] = &EmuGB::OpCB38;
	m_CBOpCodes[0x39] = &EmuGB::OpCB39;
	m_CBOpCodes[0x3A] = &EmuGB::OpCB3A;
	m_CBOpCodes[0x3B] = &EmuGB::OpCB3B;
	m_CBOpCodes[0x3C] = &EmuGB::OpCB3C;
	m_CBOpCodes[0x3D] = &EmuGB::OpCB3D;
	m_CBOpCodes[0x3E] = &EmuGB::OpCB3E;
	m_CBOpCodes[0x3F] = &EmuGB::OpCB3F;

	m_CBOpCodes[0x40] = &EmuGB::OpCB40;
	m_CBOpCodes[0x41] = &EmuGB::OpCB41;
	m_CBOpCodes[0x42] = &EmuGB::OpCB42;
	m_CBOpCodes[0x43] = &EmuGB::OpCB43;
	m_CBOpCodes[0x44] = &EmuGB::OpCB44;
	m_CBOpCodes[0x45] = &EmuGB::OpCB45;
	m_CBOpCodes[0x46] = &EmuGB::OpCB46;
	m_CBOpCodes[0x47] = &EmuGB::OpCB47;
	m_CBOpCodes[0x48] = &EmuGB::OpCB48;
	m_CBOpCodes[0x49] = &EmuGB::OpCB49;
	m_CBOpCodes[0x4A] = &EmuGB::OpCB4A;
	m_CBOpCodes[0x4B] = &EmuGB::OpCB4B;
	m_CBOpCodes[0x4C] = &EmuGB::OpCB4C;
	m_CBOpCodes[0x4D] = &EmuGB::OpCB4D;
	m_CBOpCodes[0x4E] = &EmuGB::OpCB4E;
	m_CBOpCodes[0x4F] = &EmuGB::OpCB4F;

	m_CBOpCodes[0x50] = &EmuGB::OpCB50;
	m_CBOpCodes[0x51] = &EmuGB::OpCB51;
	m_CBOpCodes[0x52] = &EmuGB::OpCB52;
	m_CBOpCodes[0x53] = &EmuGB::OpCB53;
	m_CBOpCodes[0x54] = &EmuGB::OpCB54;
	m_CBOpCodes[0x55] = &EmuGB::OpCB55;
	m_CBOpCodes[0x56] = &EmuGB::OpCB56;
	m_CBOpCodes[0x57] = &EmuGB::OpCB57;
	m_CBOpCodes[0x58] = &EmuGB::OpCB58;
	m_CBOpCodes[0x59] = &EmuGB::OpCB59;
	m_CBOpCodes[0x5A] = &EmuGB::OpCB5A;
	m_CBOpCodes[0x5B] = &EmuGB::OpCB5B;
	m_CBOpCodes[0x5C] = &EmuGB::OpCB5C;
	m_CBOpCodes[0x5D] = &EmuGB::OpCB5D;
	m_CBOpCodes[0x5E] = &EmuGB::OpCB5E;
	m_CBOpCodes[0x5F] = &EmuGB::OpCB5F;

	m_CBOpCodes[0x60] = &EmuGB::OpCB60;
	m_CBOpCodes[0x61] = &EmuGB::OpCB61;
	m_CBOpCodes[0x62] = &EmuGB::OpCB62;
	m_CBOpCodes[0x63] = &EmuGB::OpCB63;
	m_CBOpCodes[0x64] = &EmuGB::OpCB64;
	m_CBOpCodes[0x65] = &EmuGB::OpCB65;
	m_CBOpCodes[0x66] = &EmuGB::OpCB66;
	m_CBOpCodes[0x67] = &EmuGB::OpCB67;
	m_CBOpCodes[0x68] = &EmuGB::OpCB68;
	m_CBOpCodes[0x69] = &EmuGB::OpCB69;
	m_CBOpCodes[0x6A] = &EmuGB::OpCB6A;
	m_CBOpCodes[0x6B] = &EmuGB::OpCB6B;
	m_CBOpCodes[0x6C] = &EmuGB::OpCB6C;
	m_CBOpCodes[0x6D] = &EmuGB::OpCB6D;
	m_CBOpCodes[0x6E] = &EmuGB::OpCB6E;
	m_CBOpCodes[0x6F] = &EmuGB::OpCB6F;

	m_CBOpCodes[0x70] = &EmuGB::OpCB70;
	m_CBOpCodes[0x71] = &EmuGB::OpCB71;
	m_CBOpCodes[0x72] = &EmuGB::OpCB72;
	m_CBOpCodes[0x73] = &EmuGB::OpCB73;
	m_CBOpCodes[0x74] = &EmuGB::OpCB74;
	m_CBOpCodes[0x75] = &EmuGB::OpCB75;
	m_CBOpCodes[0x76] = &EmuGB::OpCB76;
	m_CBOpCodes[0x77] = &EmuGB::OpCB77;
	m_CBOpCodes[0x78] = &EmuGB::OpCB78;
	m_CBOpCodes[0x79] = &EmuGB::OpCB79;
	m_CBOpCodes[0x7A] = &EmuGB::OpCB7A;
	m_CBOpCodes[0x7B] = &EmuGB::OpCB7B;
	m_CBOpCodes[0x7C] = &EmuGB::OpCB7C;
	m_CBOpCodes[0x7D] = &EmuGB::OpCB7D;
	m_CBOpCodes[0x7E] = &EmuGB::OpCB7E;
	m_CBOpCodes[0x7F] = &EmuGB::OpCB7F;

	m_CBOpCodes[0x80] = &EmuGB::OpCB80;
	m_CBOpCodes[0x81] = &EmuGB::OpCB81;
	m_CBOpCodes[0x82] = &EmuGB::OpCB82;
	m_CBOpCodes[0x83] = &EmuGB::OpCB83;
	m_CBOpCodes[0x84] = &EmuGB::OpCB84;
	m_CBOpCodes[0x85] = &EmuGB::OpCB85;
	m_CBOpCodes[0x86] = &EmuGB::OpCB86;
	m_CBOpCodes[0x87] = &EmuGB::OpCB87;
	m_CBOpCodes[0x88] = &EmuGB::OpCB88;
	m_CBOpCodes[0x89] = &EmuGB::OpCB89;
	m_CBOpCodes[0x8A] = &EmuGB::OpCB8A;
	m_CBOpCodes[0x8B] = &EmuGB::OpCB8B;
	m_CBOpCodes[0x8C] = &EmuGB::OpCB8C;
	m_CBOpCodes[0x8D] = &EmuGB::OpCB8D;
	m_CBOpCodes[0x8E] = &EmuGB::OpCB8E;
	m_CBOpCodes[0x8F] = &EmuGB::OpCB8F;

	m_CBOpCodes[0x90] = &EmuGB::OpCB90;
	m_CBOpCodes[0x91] = &EmuGB::OpCB91;
	m_CBOpCodes[0x92] = &EmuGB::OpCB92;
	m_CBOpCodes[0x93] = &EmuGB::OpCB93;
	m_CBOpCodes[0x94] = &EmuGB::OpCB94;
	m_CBOpCodes[0x95] = &EmuGB::OpCB95;
	m_CBOpCodes[0x96] = &EmuGB::OpCB96;
	m_CBOpCodes[0x97] = &EmuGB::OpCB97;
	m_CBOpCodes[0x98] = &EmuGB::OpCB98;
	m_CBOpCodes[0x99] = &EmuGB::OpCB99;
	m_CBOpCodes[0x9A] = &EmuGB::OpCB9A;
	m_CBOpCodes[0x9B] = &EmuGB::OpCB9B;
	m_CBOpCodes[0x9C] = &EmuGB::OpCB9C;
	m_CBOpCodes[0x9D] = &EmuGB::OpCB9D;
	m_CBOpCodes[0x9E] = &EmuGB::OpCB9E;
	m_CBOpCodes[0x9F] = &EmuGB::OpCB9F;

	m_CBOpCodes[0xA0] = &EmuGB::OpCBA0;
	m_CBOpCodes[0xA1] = &EmuGB::OpCBA1;
	m_CBOpCodes[0xA2] = &EmuGB::OpCBA2;
	m_CBOpCodes[0xA3] = &EmuGB::OpCBA3;
	m_CBOpCodes[0xA4] = &EmuGB::OpCBA4;
	m_CBOpCodes[0xA5] = &EmuGB::OpCBA5;
	m_CBOpCodes[0xA6] = &EmuGB::OpCBA6;
	m_CBOpCodes[0xA7] = &EmuGB::OpCBA7;
	m_CBOpCodes[0xA8] = &EmuGB::OpCBA8;
	m_CBOpCodes[0xA9] = &EmuGB::OpCBA9;
	m_CBOpCodes[0xAA] = &EmuGB::OpCBAA;
	m_CBOpCodes[0xAB] = &EmuGB::OpCBAB;
	m_CBOpCodes[0xAC] = &EmuGB::OpCBAC;
	m_CBOpCodes[0xAD] = &EmuGB::OpCBAD;
	m_CBOpCodes[0xAE] = &EmuGB::OpCBAE;
	m_CBOpCodes[0xAF] = &EmuGB::OpCBAF;

	m_CBOpCodes[0xB0] = &EmuGB::OpCBB0;
	m_CBOpCodes[0xB1] = &EmuGB::OpCBB1;
	m_CBOpCodes[0xB2] = &EmuGB::OpCBB2;
	m_CBOpCodes[0xB3] = &EmuGB::OpCBB3;
	m_CBOpCodes[0xB4] = &EmuGB::OpCBB4;
	m_CBOpCodes[0xB5] = &EmuGB::OpCBB5;
	m_CBOpCodes[0xB6] = &EmuGB::OpCBB6;
	m_CBOpCodes[0xB7] = &EmuGB::OpCBB7;
	m_CBOpCodes[0xB8] = &EmuGB::OpCBB8;
	m_CBOpCodes[0xB9] = &EmuGB::OpCBB9;
	m_CBOpCodes[0xBA] = &EmuGB::OpCBBA;
	m_CBOpCodes[0xBB] = &EmuGB::OpCBBB;
	m_CBOpCodes[0xBC] = &EmuGB::OpCBBC;
	m_CBOpCodes[0xBD] = &EmuGB::OpCBBD;
	m_CBOpCodes[0xBE] = &EmuGB::OpCBBE;
	m_CBOpCodes[0xBF] = &EmuGB::OpCBBF;

	m_CBOpCodes[0xC0] = &EmuGB::OpCBC0;
	m_CBOpCodes[0xC1] = &EmuGB::OpCBC1;
	m_CBOpCodes[0xC2] = &EmuGB::OpCBC2;
	m_CBOpCodes[0xC3] = &EmuGB::OpCBC3;
	m_CBOpCodes[0xC4] = &EmuGB::OpCBC4;
	m_CBOpCodes[0xC5] = &EmuGB::OpCBC5;
	m_CBOpCodes[0xC6] = &EmuGB::OpCBC6;
	m_CBOpCodes[0xC7] = &EmuGB::OpCBC7;
	m_CBOpCodes[0xC8] = &EmuGB::OpCBC8;
	m_CBOpCodes[0xC9] = &EmuGB::OpCBC9;
	m_CBOpCodes[0xCA] = &EmuGB::OpCBCA;
	m_CBOpCodes[0xCB] = &EmuGB::OpCBCB;
	m_CBOpCodes[0xCC] = &EmuGB::OpCBCC;
	m_CBOpCodes[0xCD] = &EmuGB::OpCBCD;
	m_CBOpCodes[0xCE] = &EmuGB::OpCBCE;
	m_CBOpCodes[0xCF] = &EmuGB::OpCBCF;

	m_CBOpCodes[0xD0] = &EmuGB::OpCBD0;
	m_CBOpCodes[0xD1] = &EmuGB::OpCBD1;
	m_CBOpCodes[0xD2] = &EmuGB::OpCBD2;
	m_CBOpCodes[0xD3] = &EmuGB::OpCBD3;
	m_CBOpCodes[0xD4] = &EmuGB::OpCBD4;
	m_CBOpCodes[0xD5] = &EmuGB::OpCBD5;
	m_CBOpCodes[0xD6] = &EmuGB::OpCBD6;
	m_CBOpCodes[0xD7] = &EmuGB::OpCBD7;
	m_CBOpCodes[0xD8] = &EmuGB::OpCBD8;
	m_CBOpCodes[0xD9] = &EmuGB::OpCBD9;
	m_CBOpCodes[0xDA] = &EmuGB::OpCBDA;
	m_CBOpCodes[0xDB] = &EmuGB::OpCBDB;
	m_CBOpCodes[0xDC] = &EmuGB::OpCBDC;
	m_CBOpCodes[0xDD] = &EmuGB::OpCBDD;
	m_CBOpCodes[0xDE] = &EmuGB::OpCBDE;
	m_CBOpCodes[0xDF] = &EmuGB::OpCBDF;

	m_CBOpCodes[0xE0] = &EmuGB::OpCBE0;
	m_CBOpCodes[0xE1] = &EmuGB::OpCBE1;
	m_CBOpCodes[0xE2] = &EmuGB::OpCBE2;
	m_CBOpCodes[0xE3] = &EmuGB::OpCBE3;
	m_CBOpCodes[0xE4] = &EmuGB::OpCBE4;
	m_CBOpCodes[0xE5] = &EmuGB::OpCBE5;
	m_CBOpCodes[0xE6] = &EmuGB::OpCBE6;
	m_CBOpCodes[0xE7] = &EmuGB::OpCBE7;
	m_CBOpCodes[0xE8] = &EmuGB::OpCBE8;
	m_CBOpCodes[0xE9] = &EmuGB::OpCBE9;
	m_CBOpCodes[0xEA] = &EmuGB::OpCBEA;
	m_CBOpCodes[0xEB] = &EmuGB::OpCBEB;
	m_CBOpCodes[0xEC] = &EmuGB::OpCBEC;
	m_CBOpCodes[0xED] = &EmuGB::OpCBED;
	m_CBOpCodes[0xEE] = &EmuGB::OpCBEE;
	m_CBOpCodes[0xEF] = &EmuGB::OpCBEF;

	m_CBOpCodes[0xF0] = &EmuGB::OpCBF0;
	m_CBOpCodes[0xF1] = &EmuGB::OpCBF1;
	m_CBOpCodes[0xF2] = &EmuGB::OpCBF2;
	m_CBOpCodes[0xF3] = &EmuGB::OpCBF3;
	m_CBOpCodes[0xF4] = &EmuGB::OpCBF4;
	m_CBOpCodes[0xF5] = &EmuGB::OpCBF5;
	m_CBOpCodes[0xF6] = &EmuGB::OpCBF6;
	m_CBOpCodes[0xF7] = &EmuGB::OpCBF7;
	m_CBOpCodes[0xF8] = &EmuGB::OpCBF8;
	m_CBOpCodes[0xF9] = &EmuGB::OpCBF9;
	m_CBOpCodes[0xFA] = &EmuGB::OpCBFA;
	m_CBOpCodes[0xFB] = &EmuGB::OpCBFB;
	m_CBOpCodes[0xFC] = &EmuGB::OpCBFC;
	m_CBOpCodes[0xFD] = &EmuGB::OpCBFD;
	m_CBOpCodes[0xFE] = &EmuGB::OpCBFE;
	m_CBOpCodes[0xFF] = &EmuGB::OpCBFF;
}

void EmuGB::Reset()
{
	// Reset registers
	SetWordRegister<EmuGB::WordRegisters::SP_REGISTER>(0x0000);
	SetWordRegister<EmuGB::WordRegisters::PC_REGISTER>(0x0000);
	SetWordRegister<EmuGB::WordRegisters::AF_REGISTER>(0x0000);
	SetWordRegister<EmuGB::WordRegisters::BC_REGISTER>(0x0000);
	SetWordRegister<EmuGB::WordRegisters::DE_REGISTER>(0x0000);
	SetWordRegister<EmuGB::WordRegisters::HL_REGISTER>(0x0000);


	for (unsigned int i = 0; i < 0xFFFF; ++i)
	{
		m_bus_memory[i] = 0x0;
	}

	// Reset BIOS
	memcpy(m_bus_memory, bootDMG, bootDMGSize);
}

bool EmuGB::InMemoryRange(ui16 start, ui16 end, ui16 address)
{
	return start <= address && end >= address;
}

ui8 EmuGB::GetCycleModifier(ui8 cycle)
{
	return cycle >> m_cycle_modifier;
}

ui8 EmuGB::ReadByteFromPC()
{
	return ProcessBusReadRef<ui16,ui8>(GetWordRegister<EmuGB::WordRegisters::PC_REGISTER>()++);
}

ui16 EmuGB::ReadWordFromPC()
{
	ui16 data = 0;

	ReadWordFromPC(data);

	return data;
}

void EmuGB::ReadWordFromPC(ui16& data)
{
	ProcessMemory<MemoryAccessType::Read, ui8, ui16, ui16>(m_bus_memory, GetWordRegister<EmuGB::WordRegisters::PC_REGISTER>(), data);

	GetWordRegister<EmuGB::WordRegisters::PC_REGISTER>() += 2;
}

void EmuGB::Jr()
{
	GetWordRegister<WordRegisters::PC_REGISTER>() += static_cast<i8>(ReadByteFromPC());
}

void EmuGB::Bit(const ui8& value, ui8 bit)
{
	// clear flags
	GetByteRegister<EmuGB::ByteRegisters::F_REGISTER>() = 0;

	// Process Bit comparison
	SetFlag<EmuGB::Flags::FLAG_ZERO>(((value >> bit) & 0x01) == 0);
	SetFlag<EmuGB::Flags::FLAG_HALF_CARRY, true>();
}

void EmuGB::XOR(const ui8& value)
{
	ui8& data = GetByteRegister<EmuGB::ByteRegisters::A_REGISTER>();
	data ^= value;

	// clear flags
	GetByteRegister<EmuGB::ByteRegisters::F_REGISTER>() = 0;
	// If Zero
	SetFlag<EmuGB::Flags::FLAG_ZERO>(data == 0);
}

void EmuGB::rl(ui8& value, bool a_reg)
{
	ui8 carry = GetFlag<Flags::FLAG_CARRY>() ? 1 : 0;

	GetByteRegister<ByteRegisters::F_REGISTER>() = 0;

	if ((value & 0x80) != 0)
	{
		SetFlag<Flags::FLAG_CARRY, true>();
	}

	value <<= 1;

	value |= carry;
	if (!a_reg)
	{
		SetFlag<Flags::FLAG_ZERO>(value == 0);
	}
}

void EmuGB::Cp(const ui8& value)
{
	ui8& reg = GetByteRegister<ByteRegisters::A_REGISTER>();

	SetFlag<Flags::FLAG_SUBTRACT, true>();
	SetFlag<Flags::FLAG_CARRY>(reg < value);
	SetFlag<Flags::FLAG_ZERO>(reg == value);
	SetFlag<Flags::FLAG_HALF_CARRY>(((reg - value) & 0xF) > (reg & 0xF));
}

void EmuGB::Sub(const ui8& value)
{
	int current_register = GetByteRegister<ByteRegisters::A_REGISTER>();
	int result = current_register - value;
	int carrybits = current_register ^ value ^ result;

	GetByteRegister<ByteRegisters::A_REGISTER>() = static_cast<ui8>(result);

	// Clear flags
	GetByteRegister<ByteRegisters::F_REGISTER>() = 0;

	SetFlag<Flags::FLAG_SUBTRACT, true>();

	SetFlag< Flags::FLAG_ZERO>( static_cast<ui8>(result) == 0);

	if ((carrybits & 0x100) != 0)
	{
		SetFlag<Flags::FLAG_CARRY, true>();
	}
	if ((carrybits & 0x10) != 0)
	{
		SetFlag<Flags::FLAG_HALF_CARRY, true>();
	}
}

void EmuGB::Op00() { assert("Missing" && 0); }
void EmuGB::Op01() { SetWordRegister<WordRegisters::BC_REGISTER>(ReadWordFromPC()); }
void EmuGB::Op02() { assert("Missing" && 0); }
void EmuGB::Op03() { assert("Missing" && 0); }
void EmuGB::Op04() { INCByteRegister<ByteRegisters::B_REGISTER>(); }
void EmuGB::Op05() { DECByteRegister<ByteRegisters::B_REGISTER>(); }
void EmuGB::Op06() { GetByteRegister<ByteRegisters::B_REGISTER>() = ReadByteFromPC(); }
void EmuGB::Op07() { assert("Missing" && 0); }
void EmuGB::Op08() { assert("Missing" && 0); }
void EmuGB::Op09() { assert("Missing" && 0); }
void EmuGB::Op0A() { assert("Missing" && 0); }
void EmuGB::Op0B() { assert("Missing" && 0); }
void EmuGB::Op0C() { INCByteRegister<ByteRegisters::C_REGISTER>(); }
void EmuGB::Op0D() { DECByteRegister<ByteRegisters::C_REGISTER>(); }
void EmuGB::Op0E() { GetByteRegister<ByteRegisters::C_REGISTER>() = ReadByteFromPC(); }
void EmuGB::Op0F() { assert("Missing" && 0); }

void EmuGB::Op10() { assert("Missing" && 0); }
void EmuGB::Op11() { SetWordRegister<WordRegisters::DE_REGISTER>(ReadWordFromPC()); }
void EmuGB::Op12() { assert("Missing" && 0); }
void EmuGB::Op13() { GetWordRegister<WordRegisters::DE_REGISTER>()++; }
void EmuGB::Op14() { assert("Missing" && 0); }
void EmuGB::Op15() { assert("Missing" && 0); }
void EmuGB::Op16() { assert("Missing" && 0); }
void EmuGB::Op17() { rl(GetByteRegister<ByteRegisters::A_REGISTER>(), true); }
void EmuGB::Op18() { Jr(); }
void EmuGB::Op19() { assert("Missing" && 0); }
void EmuGB::Op1A() { ProcessBus<MemoryAccessType::Read, ui16, ui8>(GetWordRegister<WordRegisters::DE_REGISTER>(), GetByteRegister<ByteRegisters::A_REGISTER>()); }
void EmuGB::Op1B() { assert("Missing" && 0); }
void EmuGB::Op1C() { assert("Missing" && 0); }
void EmuGB::Op1D() { assert("Missing" && 0); }
void EmuGB::Op1E() { GetByteRegister<ByteRegisters::E_REGISTER>() = ReadByteFromPC(); }
void EmuGB::Op1F() { assert("Missing" && 0); }

void EmuGB::Op20() {
	if (!GetFlag<Flags::FLAG_ZERO>())
		Jr();
	else
		ReadByteFromPC();
}
void EmuGB::Op21() { ReadWordFromPC(GetWordRegister<WordRegisters::HL_REGISTER>()); }
void EmuGB::Op22() { ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>()++, GetByteRegister<ByteRegisters::A_REGISTER>()); }
void EmuGB::Op23() { GetWordRegister<WordRegisters::HL_REGISTER>()++; }
void EmuGB::Op24() { assert("Missing" && 0); }
void EmuGB::Op25() { assert("Missing" && 0); }
void EmuGB::Op26() { GetByteRegister<ByteRegisters::H_REGISTER>() = ReadByteFromPC(); }
void EmuGB::Op27() { assert("Missing" && 0); }
void EmuGB::Op28() {
	if (GetFlag<Flags::FLAG_ZERO>())
		Jr();
	else
		ReadByteFromPC();
}
void EmuGB::Op29() { assert("Missing" && 0); }
void EmuGB::Op2A() { assert("Missing" && 0); }
void EmuGB::Op2B() { assert("Missing" && 0); }
void EmuGB::Op2C() { assert("Missing" && 0); }
void EmuGB::Op2D() { assert("Missing" && 0); }
void EmuGB::Op2E() { GetByteRegister<ByteRegisters::L_REGISTER>() = ReadByteFromPC(); }
void EmuGB::Op2F() { assert("Missing" && 0); }

void EmuGB::Op30() { assert("Missing" && 0); }
void EmuGB::Op31() { ReadWordFromPC(GetWordRegister<WordRegisters::SP_REGISTER>()); }
void EmuGB::Op32() { ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>()--, GetByteRegister<ByteRegisters::A_REGISTER>()); }
void EmuGB::Op33() { assert("Missing" && 0); }
void EmuGB::Op34() { assert("Missing" && 0); }
void EmuGB::Op35() { assert("Missing" && 0); }
void EmuGB::Op36() { assert("Missing" && 0); }
void EmuGB::Op37() { assert("Missing" && 0); }
void EmuGB::Op38() { assert("Missing" && 0); }
void EmuGB::Op39() { assert("Missing" && 0); }
void EmuGB::Op3A() { assert("Missing" && 0); }
void EmuGB::Op3B() { assert("Missing" && 0); }
void EmuGB::Op3C() { assert("Missing" && 0); }
void EmuGB::Op3D() { DECByteRegister<ByteRegisters::A_REGISTER>(); }
void EmuGB::Op3E() { GetByteRegister<ByteRegisters::A_REGISTER>() = ReadByteFromPC(); }
void EmuGB::Op3F() { assert("Missing" && 0); }

void EmuGB::Op40() { assert("Missing" && 0); }
void EmuGB::Op41() { assert("Missing" && 0); }
void EmuGB::Op42() { assert("Missing" && 0); }
void EmuGB::Op43() { assert("Missing" && 0); }
void EmuGB::Op44() { assert("Missing" && 0); }
void EmuGB::Op45() { assert("Missing" && 0); }
void EmuGB::Op46() { assert("Missing" && 0); }
void EmuGB::Op47() { assert("Missing" && 0); }
void EmuGB::Op48() { assert("Missing" && 0); }
void EmuGB::Op49() { assert("Missing" && 0); }
void EmuGB::Op4A() { assert("Missing" && 0); }
void EmuGB::Op4B() { assert("Missing" && 0); }
void EmuGB::Op4C() { assert("Missing" && 0); }
void EmuGB::Op4D() { assert("Missing" && 0); }
void EmuGB::Op4E() { assert("Missing" && 0); }
void EmuGB::Op4F() { GetByteRegister<ByteRegisters::C_REGISTER>() = GetByteRegister<ByteRegisters::A_REGISTER>(); }

void EmuGB::Op50() { assert("Missing" && 0); }
void EmuGB::Op51() { assert("Missing" && 0); }
void EmuGB::Op52() { assert("Missing" && 0); }
void EmuGB::Op53() { assert("Missing" && 0); }
void EmuGB::Op54() { assert("Missing" && 0); }
void EmuGB::Op55() { assert("Missing" && 0); }
void EmuGB::Op56() { assert("Missing" && 0); }
void EmuGB::Op57() { GetByteRegister<ByteRegisters::D_REGISTER>() = GetByteRegister<ByteRegisters::A_REGISTER>(); }
void EmuGB::Op58() { assert("Missing" && 0); }
void EmuGB::Op59() { assert("Missing" && 0); }
void EmuGB::Op5A() { assert("Missing" && 0); }
void EmuGB::Op5B() { assert("Missing" && 0); }
void EmuGB::Op5C() { assert("Missing" && 0); }
void EmuGB::Op5D() { assert("Missing" && 0); }
void EmuGB::Op5E() { assert("Missing" && 0); }
void EmuGB::Op5F() { assert("Missing" && 0); }

void EmuGB::Op60() { assert("Missing" && 0); }
void EmuGB::Op61() { assert("Missing" && 0); }
void EmuGB::Op62() { assert("Missing" && 0); }
void EmuGB::Op63() { assert("Missing" && 0); }
void EmuGB::Op64() { assert("Missing" && 0); }
void EmuGB::Op65() { assert("Missing" && 0); }
void EmuGB::Op66() { assert("Missing" && 0); }
void EmuGB::Op67() { GetByteRegister<ByteRegisters::H_REGISTER>() = GetByteRegister<ByteRegisters::A_REGISTER>(); }
void EmuGB::Op68() { assert("Missing" && 0); }
void EmuGB::Op69() { assert("Missing" && 0); }
void EmuGB::Op6A() { assert("Missing" && 0); }
void EmuGB::Op6B() { assert("Missing" && 0); }
void EmuGB::Op6C() { assert("Missing" && 0); }
void EmuGB::Op6D() { assert("Missing" && 0); }
void EmuGB::Op6E() { assert("Missing" && 0); }
void EmuGB::Op6F() { assert("Missing" && 0); }

void EmuGB::Op70() { assert("Missing" && 0); }
void EmuGB::Op71() { assert("Missing" && 0); }
void EmuGB::Op72() { assert("Missing" && 0); }
void EmuGB::Op73() { assert("Missing" && 0); }
void EmuGB::Op74() { assert("Missing" && 0); }
void EmuGB::Op75() { assert("Missing" && 0); }
void EmuGB::Op76() { assert("Missing" && 0); }
void EmuGB::Op77() { ProcessBus<MemoryAccessType::Write, ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>(), GetByteRegister<ByteRegisters::A_REGISTER>()); }
void EmuGB::Op78() { assert("Missing" && 0); }
void EmuGB::Op79() { assert("Missing" && 0); }
void EmuGB::Op7A() { assert("Missing" && 0); }
void EmuGB::Op7B() { GetByteRegister<ByteRegisters::A_REGISTER>() = GetByteRegister<ByteRegisters::E_REGISTER>(); }
void EmuGB::Op7C() { assert("Missing" && 0); }
void EmuGB::Op7D() { assert("Missing" && 0); }
void EmuGB::Op7E() { assert("Missing" && 0); }
void EmuGB::Op7F() { assert("Missing" && 0); }

void EmuGB::Op80() { assert("Missing" && 0); }
void EmuGB::Op81() { assert("Missing" && 0); }
void EmuGB::Op82() { assert("Missing" && 0); }
void EmuGB::Op83() { assert("Missing" && 0); }
void EmuGB::Op84() { assert("Missing" && 0); }
void EmuGB::Op85() { assert("Missing" && 0); }
void EmuGB::Op86() { assert("Missing" && 0); }
void EmuGB::Op87() { assert("Missing" && 0); }
void EmuGB::Op88() { assert("Missing" && 0); }
void EmuGB::Op89() { assert("Missing" && 0); }
void EmuGB::Op8A() { assert("Missing" && 0); }
void EmuGB::Op8B() { assert("Missing" && 0); }
void EmuGB::Op8C() { assert("Missing" && 0); }
void EmuGB::Op8D() { assert("Missing" && 0); }
void EmuGB::Op8E() { assert("Missing" && 0); }
void EmuGB::Op8F() { assert("Missing" && 0); }

void EmuGB::Op90() { Sub(GetByteRegister<ByteRegisters::B_REGISTER>()); }
void EmuGB::Op91() { assert("Missing" && 0); }
void EmuGB::Op92() { assert("Missing" && 0); }
void EmuGB::Op93() { assert("Missing" && 0); }
void EmuGB::Op94() { assert("Missing" && 0); }
void EmuGB::Op95() { assert("Missing" && 0); }
void EmuGB::Op96() { assert("Missing" && 0); }
void EmuGB::Op97() { assert("Missing" && 0); }
void EmuGB::Op98() { assert("Missing" && 0); }
void EmuGB::Op99() { assert("Missing" && 0); }
void EmuGB::Op9A() { assert("Missing" && 0); }
void EmuGB::Op9B() { assert("Missing" && 0); }
void EmuGB::Op9C() { assert("Missing" && 0); }
void EmuGB::Op9D() { assert("Missing" && 0); }
void EmuGB::Op9E() { assert("Missing" && 0); }
void EmuGB::Op9F() { assert("Missing" && 0); }

void EmuGB::OpA0() { assert("Missing" && 0); }
void EmuGB::OpA1() { assert("Missing" && 0); }
void EmuGB::OpA2() { assert("Missing" && 0); }
void EmuGB::OpA3() { assert("Missing" && 0); }
void EmuGB::OpA4() { assert("Missing" && 0); }
void EmuGB::OpA5() { assert("Missing" && 0); }
void EmuGB::OpA6() { assert("Missing" && 0); }
void EmuGB::OpA7() { assert("Missing" && 0); }
void EmuGB::OpA8() { assert("Missing" && 0); }
void EmuGB::OpA9() { assert("Missing" && 0); }
void EmuGB::OpAA() { assert("Missing" && 0); }
void EmuGB::OpAB() { assert("Missing" && 0); }
void EmuGB::OpAC() { assert("Missing" && 0); }
void EmuGB::OpAD() { assert("Missing" && 0); }
void EmuGB::OpAE() { assert("Missing" && 0); }
void EmuGB::OpAF() { XOR(GetByteRegister<EmuGB::ByteRegisters::A_REGISTER>()); }

void EmuGB::OpB0() { assert("Missing" && 0); }
void EmuGB::OpB1() { assert("Missing" && 0); }
void EmuGB::OpB2() { assert("Missing" && 0); }
void EmuGB::OpB3() { assert("Missing" && 0); }
void EmuGB::OpB4() { assert("Missing" && 0); }
void EmuGB::OpB5() { assert("Missing" && 0); }
void EmuGB::OpB6() { assert("Missing" && 0); }
void EmuGB::OpB7() { assert("Missing" && 0); }
void EmuGB::OpB8() { assert("Missing" && 0); }
void EmuGB::OpB9() { assert("Missing" && 0); }
void EmuGB::OpBA() { assert("Missing" && 0); }
void EmuGB::OpBB() { assert("Missing" && 0); }
void EmuGB::OpBC() { assert("Missing" && 0); }
void EmuGB::OpBD() { assert("Missing" && 0); }
void EmuGB::OpBE() { assert("Missing" && 0); }
void EmuGB::OpBF() { assert("Missing" && 0); }

void EmuGB::OpC0() { assert("Missing" && 0); }
void EmuGB::OpC1() { StackPop<WordRegisters::BC_REGISTER>(); }
void EmuGB::OpC2() { assert("Missing" && 0); }
void EmuGB::OpC3() { assert("Missing" && 0); }
void EmuGB::OpC4() { assert("Missing" && 0); }
void EmuGB::OpC5() { StackPush<WordRegisters::BC_REGISTER>(); }
void EmuGB::OpC6() { assert("Missing" && 0); }
void EmuGB::OpC7() { assert("Missing" && 0); }
void EmuGB::OpC8() { assert("Missing" && 0); }
void EmuGB::OpC9() { StackPop<WordRegisters::PC_REGISTER>(); }
void EmuGB::OpCA() { assert("Missing" && 0); }
//void EmuGB::OpCB() { assert("Missing" && 0); }
void EmuGB::OpCC() { assert("Missing" && 0); }
void EmuGB::OpCD() {

	ui16 newAddress = ReadWordFromPC();
	StackPush<WordRegisters::PC_REGISTER>();
	SetWordRegister<WordRegisters::PC_REGISTER>(newAddress);
}
void EmuGB::OpCE() { assert("Missing" && 0); }
void EmuGB::OpCF() { assert("Missing" && 0); }

void EmuGB::OpD0() { assert("Missing" && 0); }
void EmuGB::OpD1() { assert("Missing" && 0); }
void EmuGB::OpD2() { assert("Missing" && 0); }
void EmuGB::OpD3() { assert("Missing" && 0); }
void EmuGB::OpD4() { assert("Missing" && 0); }
void EmuGB::OpD5() { assert("Missing" && 0); }
void EmuGB::OpD6() { assert("Missing" && 0); }
void EmuGB::OpD7() { assert("Missing" && 0); }
void EmuGB::OpD8() { assert("Missing" && 0); }
void EmuGB::OpD9() { assert("Missing" && 0); }
void EmuGB::OpDA() { assert("Missing" && 0); }
void EmuGB::OpDB() { assert("Missing" && 0); }
void EmuGB::OpDC() { assert("Missing" && 0); }
void EmuGB::OpDD() { assert("Missing" && 0); }
void EmuGB::OpDE() { assert("Missing" && 0); }
void EmuGB::OpDF() { assert("Missing" && 0); }

void EmuGB::OpE0() { ProcessBus<MemoryAccessType::Write, ui16, ui8>(0xFF00 + ReadByteFromPC(), GetByteRegister<ByteRegisters::A_REGISTER>()); }
void EmuGB::OpE1() { assert("Missing" && 0); }
void EmuGB::OpE2() { ProcessBus<MemoryAccessType::Write, ui16, ui8>(0xFF00 + GetByteRegister<ByteRegisters::C_REGISTER>(), GetByteRegister<ByteRegisters::A_REGISTER>()); }
void EmuGB::OpE3() { assert("Missing" && 0); }
void EmuGB::OpE4() { assert("Missing" && 0); }
void EmuGB::OpE5() { assert("Missing" && 0); }
void EmuGB::OpE6() { assert("Missing" && 0); }
void EmuGB::OpE7() { assert("Missing" && 0); }
void EmuGB::OpE8() { assert("Missing" && 0); }
void EmuGB::OpE9() { assert("Missing" && 0); }
void EmuGB::OpEA() { ProcessBus<MemoryAccessType::Write, ui16, ui8>(ReadWordFromPC(), GetByteRegister<ByteRegisters::A_REGISTER>()); }
void EmuGB::OpEB() { assert("Missing" && 0); }
void EmuGB::OpEC() { assert("Missing" && 0); }
void EmuGB::OpED() { assert("Missing" && 0); }
void EmuGB::OpEE() { assert("Missing" && 0); }
void EmuGB::OpEF() { assert("Missing" && 0); }

void EmuGB::OpF0() { ProcessBus<MemoryAccessType::Read, ui16, ui8>(0xFF00 + ReadWordFromPC(), GetByteRegister<ByteRegisters::A_REGISTER>()); }
void EmuGB::OpF1() { assert("Missing" && 0); }
void EmuGB::OpF2() { assert("Missing" && 0); }
void EmuGB::OpF3() { assert("Missing" && 0); }
void EmuGB::OpF4() { assert("Missing" && 0); }
void EmuGB::OpF5() { assert("Missing" && 0); }
void EmuGB::OpF6() { assert("Missing" && 0); }
void EmuGB::OpF7() { assert("Missing" && 0); }
void EmuGB::OpF8() { assert("Missing" && 0); }
void EmuGB::OpF9() { assert("Missing" && 0); }
void EmuGB::OpFA() { assert("Missing" && 0); }
void EmuGB::OpFB() {
	m_interrupts_enabled = true;
	m_IECycles = m_cycles[0xFB] + GetCycleModifier(4);
}
void EmuGB::OpFC() { assert("Missing" && 0); }
void EmuGB::OpFD() { assert("Missing" && 0); }
void EmuGB::OpFE() { Cp(ReadByteFromPC()); }
void EmuGB::OpFF() { assert("Missing" && 0); }


void EmuGB::OpCB00() { assert("Missing" && 0); }
void EmuGB::OpCB01() { assert("Missing" && 0); }
void EmuGB::OpCB02() { assert("Missing" && 0); }
void EmuGB::OpCB03() { assert("Missing" && 0); }
void EmuGB::OpCB04() { assert("Missing" && 0); }
void EmuGB::OpCB05() { assert("Missing" && 0); }
void EmuGB::OpCB06() { assert("Missing" && 0); }
void EmuGB::OpCB07() { assert("Missing" && 0); }
void EmuGB::OpCB08() { assert("Missing" && 0); }
void EmuGB::OpCB09() { assert("Missing" && 0); }
void EmuGB::OpCB0A() { assert("Missing" && 0); }
void EmuGB::OpCB0B() { assert("Missing" && 0); }
void EmuGB::OpCB0C() { assert("Missing" && 0); }
void EmuGB::OpCB0D() { assert("Missing" && 0); }
void EmuGB::OpCB0E() { assert("Missing" && 0); }
void EmuGB::OpCB0F() { assert("Missing" && 0); }

void EmuGB::OpCB10() { rl(GetByteRegister<ByteRegisters::B_REGISTER>()); }
void EmuGB::OpCB11() { rl(GetByteRegister<ByteRegisters::C_REGISTER>()); }
void EmuGB::OpCB12() { rl(GetByteRegister<ByteRegisters::D_REGISTER>()); }
void EmuGB::OpCB13() { rl(GetByteRegister<ByteRegisters::E_REGISTER>()); }
void EmuGB::OpCB14() { rl(GetByteRegister<ByteRegisters::H_REGISTER>()); }
void EmuGB::OpCB15() { rl(GetByteRegister<ByteRegisters::L_REGISTER>()); }
void EmuGB::OpCB16() { rl(ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>())); }
void EmuGB::OpCB17() { rl(GetByteRegister<ByteRegisters::A_REGISTER>()); }
void EmuGB::OpCB18() { assert("Missing" && 0); }
void EmuGB::OpCB19() { assert("Missing" && 0); }
void EmuGB::OpCB1A() { assert("Missing" && 0); }
void EmuGB::OpCB1B() { assert("Missing" && 0); }
void EmuGB::OpCB1C() { assert("Missing" && 0); }
void EmuGB::OpCB1D() { assert("Missing" && 0); }
void EmuGB::OpCB1E() { assert("Missing" && 0); }
void EmuGB::OpCB1F() { assert("Missing" && 0); }

void EmuGB::OpCB20() { assert("Missing" && 0); }
void EmuGB::OpCB21() { assert("Missing" && 0); }
void EmuGB::OpCB22() { assert("Missing" && 0); }
void EmuGB::OpCB23() { assert("Missing" && 0); }
void EmuGB::OpCB24() { assert("Missing" && 0); }
void EmuGB::OpCB25() { assert("Missing" && 0); }
void EmuGB::OpCB26() { assert("Missing" && 0); }
void EmuGB::OpCB27() { assert("Missing" && 0); }
void EmuGB::OpCB28() { assert("Missing" && 0); }
void EmuGB::OpCB29() { assert("Missing" && 0); }
void EmuGB::OpCB2A() { assert("Missing" && 0); }
void EmuGB::OpCB2B() { assert("Missing" && 0); }
void EmuGB::OpCB2C() { assert("Missing" && 0); }
void EmuGB::OpCB2D() { assert("Missing" && 0); }
void EmuGB::OpCB2E() { assert("Missing" && 0); }
void EmuGB::OpCB2F() { assert("Missing" && 0); }

void EmuGB::OpCB30() { assert("Missing" && 0); }
void EmuGB::OpCB31() { assert("Missing" && 0); }
void EmuGB::OpCB32() { assert("Missing" && 0); }
void EmuGB::OpCB33() { assert("Missing" && 0); }
void EmuGB::OpCB34() { assert("Missing" && 0); }
void EmuGB::OpCB35() { assert("Missing" && 0); }
void EmuGB::OpCB36() { assert("Missing" && 0); }
void EmuGB::OpCB37() { assert("Missing" && 0); }
void EmuGB::OpCB38() { assert("Missing" && 0); }
void EmuGB::OpCB39() { assert("Missing" && 0); }
void EmuGB::OpCB3A() { assert("Missing" && 0); }
void EmuGB::OpCB3B() { assert("Missing" && 0); }
void EmuGB::OpCB3C() { assert("Missing" && 0); }
void EmuGB::OpCB3D() { assert("Missing" && 0); }
void EmuGB::OpCB3E() { assert("Missing" && 0); }
void EmuGB::OpCB3F() { assert("Missing" && 0); }

void EmuGB::OpCB40() { Bit(GetByteRegister<ByteRegisters::B_REGISTER>(), 0); }
void EmuGB::OpCB41() { Bit(GetByteRegister<ByteRegisters::C_REGISTER>(), 0); }
void EmuGB::OpCB42() { Bit(GetByteRegister<ByteRegisters::D_REGISTER>(), 0); }
void EmuGB::OpCB43() { Bit(GetByteRegister<ByteRegisters::E_REGISTER>(), 0); }
void EmuGB::OpCB44() { Bit(GetByteRegister<ByteRegisters::H_REGISTER>(), 0); }
void EmuGB::OpCB45() { Bit(GetByteRegister<ByteRegisters::L_REGISTER>(), 0); }
void EmuGB::OpCB46() { Bit(ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>()), 0); }
void EmuGB::OpCB47() { Bit(GetByteRegister<ByteRegisters::A_REGISTER>(), 0); }
void EmuGB::OpCB48() { Bit(GetByteRegister<ByteRegisters::B_REGISTER>(), 1); }
void EmuGB::OpCB49() { Bit(GetByteRegister<ByteRegisters::C_REGISTER>(), 1); }
void EmuGB::OpCB4A() { Bit(GetByteRegister<ByteRegisters::D_REGISTER>(), 1); }
void EmuGB::OpCB4B() { Bit(GetByteRegister<ByteRegisters::E_REGISTER>(), 1); }
void EmuGB::OpCB4C() { Bit(GetByteRegister<ByteRegisters::H_REGISTER>(), 1); }
void EmuGB::OpCB4D() { Bit(GetByteRegister<ByteRegisters::L_REGISTER>(), 1); }
void EmuGB::OpCB4E() { Bit(ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>()), 1); }
void EmuGB::OpCB4F() { Bit(GetByteRegister<ByteRegisters::A_REGISTER>(), 1); }

void EmuGB::OpCB50() { Bit(GetByteRegister<ByteRegisters::B_REGISTER>(), 2); }
void EmuGB::OpCB51() { Bit(GetByteRegister<ByteRegisters::C_REGISTER>(), 2); }
void EmuGB::OpCB52() { Bit(GetByteRegister<ByteRegisters::D_REGISTER>(), 2); }
void EmuGB::OpCB53() { Bit(GetByteRegister<ByteRegisters::E_REGISTER>(), 2); }
void EmuGB::OpCB54() { Bit(GetByteRegister<ByteRegisters::H_REGISTER>(), 2); }
void EmuGB::OpCB55() { Bit(GetByteRegister<ByteRegisters::L_REGISTER>(), 2); }
void EmuGB::OpCB56() { Bit(ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>()), 2); }
void EmuGB::OpCB57() { Bit(GetByteRegister<ByteRegisters::A_REGISTER>(), 2); }
void EmuGB::OpCB58() { Bit(GetByteRegister<ByteRegisters::B_REGISTER>(), 3); }
void EmuGB::OpCB59() { Bit(GetByteRegister<ByteRegisters::C_REGISTER>(), 3); }
void EmuGB::OpCB5A() { Bit(GetByteRegister<ByteRegisters::D_REGISTER>(), 3); }
void EmuGB::OpCB5B() { Bit(GetByteRegister<ByteRegisters::E_REGISTER>(), 3); }
void EmuGB::OpCB5C() { Bit(GetByteRegister<ByteRegisters::H_REGISTER>(), 3); }
void EmuGB::OpCB5D() { Bit(GetByteRegister<ByteRegisters::L_REGISTER>(), 3); }
void EmuGB::OpCB5E() { Bit(ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>()), 3); }
void EmuGB::OpCB5F() { Bit(GetByteRegister<ByteRegisters::A_REGISTER>(), 3); }

void EmuGB::OpCB60() { Bit(GetByteRegister<ByteRegisters::B_REGISTER>(), 4); }
void EmuGB::OpCB61() { Bit(GetByteRegister<ByteRegisters::C_REGISTER>(), 4); }
void EmuGB::OpCB62() { Bit(GetByteRegister<ByteRegisters::D_REGISTER>(), 4); }
void EmuGB::OpCB63() { Bit(GetByteRegister<ByteRegisters::E_REGISTER>(), 4); }
void EmuGB::OpCB64() { Bit(GetByteRegister<ByteRegisters::H_REGISTER>(), 4); }
void EmuGB::OpCB65() { Bit(GetByteRegister<ByteRegisters::L_REGISTER>(), 4); }
void EmuGB::OpCB66() { Bit(ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>()), 4); }
void EmuGB::OpCB67() { Bit(GetByteRegister<ByteRegisters::A_REGISTER>(), 4); }
void EmuGB::OpCB68() { Bit(GetByteRegister<ByteRegisters::B_REGISTER>(), 5); }
void EmuGB::OpCB69() { Bit(GetByteRegister<ByteRegisters::C_REGISTER>(), 5); }
void EmuGB::OpCB6A() { Bit(GetByteRegister<ByteRegisters::D_REGISTER>(), 5); }
void EmuGB::OpCB6B() { Bit(GetByteRegister<ByteRegisters::E_REGISTER>(), 5); }
void EmuGB::OpCB6C() { Bit(GetByteRegister<ByteRegisters::H_REGISTER>(), 5); }
void EmuGB::OpCB6D() { Bit(GetByteRegister<ByteRegisters::L_REGISTER>(), 5); }
void EmuGB::OpCB6E() { Bit(ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>()), 5); }
void EmuGB::OpCB6F() { Bit(GetByteRegister<ByteRegisters::A_REGISTER>(), 5); }

void EmuGB::OpCB70() { Bit(GetByteRegister<ByteRegisters::B_REGISTER>(), 6); }
void EmuGB::OpCB71() { Bit(GetByteRegister<ByteRegisters::C_REGISTER>(), 6); }
void EmuGB::OpCB72() { Bit(GetByteRegister<ByteRegisters::D_REGISTER>(), 6); }
void EmuGB::OpCB73() { Bit(GetByteRegister<ByteRegisters::E_REGISTER>(), 6); }
void EmuGB::OpCB74() { Bit(GetByteRegister<ByteRegisters::H_REGISTER>(), 6); }
void EmuGB::OpCB75() { Bit(GetByteRegister<ByteRegisters::L_REGISTER>(), 6); }
void EmuGB::OpCB76() { Bit(ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>()), 6); }
void EmuGB::OpCB77() { Bit(GetByteRegister<ByteRegisters::A_REGISTER>(), 6); }
void EmuGB::OpCB78() { Bit(GetByteRegister<ByteRegisters::B_REGISTER>(), 7); }
void EmuGB::OpCB79() { Bit(GetByteRegister<ByteRegisters::C_REGISTER>(), 7); }
void EmuGB::OpCB7A() { Bit(GetByteRegister<ByteRegisters::D_REGISTER>(), 7); }
void EmuGB::OpCB7B() { Bit(GetByteRegister<ByteRegisters::E_REGISTER>(), 7); }
void EmuGB::OpCB7C() { Bit(GetByteRegister<ByteRegisters::H_REGISTER>(), 7); }
void EmuGB::OpCB7D() { Bit(GetByteRegister<ByteRegisters::L_REGISTER>(), 7); }
void EmuGB::OpCB7E() { Bit(ProcessBusReadRef<ui16, ui8>(GetWordRegister<WordRegisters::HL_REGISTER>()), 7); }
void EmuGB::OpCB7F() { Bit(GetByteRegister<ByteRegisters::A_REGISTER>(), 7); }

void EmuGB::OpCB80() { assert("Missing" && 0); }
void EmuGB::OpCB81() { assert("Missing" && 0); }
void EmuGB::OpCB82() { assert("Missing" && 0); }
void EmuGB::OpCB83() { assert("Missing" && 0); }
void EmuGB::OpCB84() { assert("Missing" && 0); }
void EmuGB::OpCB85() { assert("Missing" && 0); }
void EmuGB::OpCB86() { assert("Missing" && 0); }
void EmuGB::OpCB87() { assert("Missing" && 0); }
void EmuGB::OpCB88() { assert("Missing" && 0); }
void EmuGB::OpCB89() { assert("Missing" && 0); }
void EmuGB::OpCB8A() { assert("Missing" && 0); }
void EmuGB::OpCB8B() { assert("Missing" && 0); }
void EmuGB::OpCB8C() { assert("Missing" && 0); }
void EmuGB::OpCB8D() { assert("Missing" && 0); }
void EmuGB::OpCB8E() { assert("Missing" && 0); }
void EmuGB::OpCB8F() { assert("Missing" && 0); }

void EmuGB::OpCB90() { assert("Missing" && 0); }
void EmuGB::OpCB91() { assert("Missing" && 0); }
void EmuGB::OpCB92() { assert("Missing" && 0); }
void EmuGB::OpCB93() { assert("Missing" && 0); }
void EmuGB::OpCB94() { assert("Missing" && 0); }
void EmuGB::OpCB95() { assert("Missing" && 0); }
void EmuGB::OpCB96() { assert("Missing" && 0); }
void EmuGB::OpCB97() { assert("Missing" && 0); }
void EmuGB::OpCB98() { assert("Missing" && 0); }
void EmuGB::OpCB99() { assert("Missing" && 0); }
void EmuGB::OpCB9A() { assert("Missing" && 0); }
void EmuGB::OpCB9B() { assert("Missing" && 0); }
void EmuGB::OpCB9C() { assert("Missing" && 0); }
void EmuGB::OpCB9D() { assert("Missing" && 0); }
void EmuGB::OpCB9E() { assert("Missing" && 0); }
void EmuGB::OpCB9F() { assert("Missing" && 0); }

void EmuGB::OpCBA0() { assert("Missing" && 0); }
void EmuGB::OpCBA1() { assert("Missing" && 0); }
void EmuGB::OpCBA2() { assert("Missing" && 0); }
void EmuGB::OpCBA3() { assert("Missing" && 0); }
void EmuGB::OpCBA4() { assert("Missing" && 0); }
void EmuGB::OpCBA5() { assert("Missing" && 0); }
void EmuGB::OpCBA6() { assert("Missing" && 0); }
void EmuGB::OpCBA7() { assert("Missing" && 0); }
void EmuGB::OpCBA8() { assert("Missing" && 0); }
void EmuGB::OpCBA9() { assert("Missing" && 0); }
void EmuGB::OpCBAA() { assert("Missing" && 0); }
void EmuGB::OpCBAB() { assert("Missing" && 0); }
void EmuGB::OpCBAC() { assert("Missing" && 0); }
void EmuGB::OpCBAD() { assert("Missing" && 0); }
void EmuGB::OpCBAE() { assert("Missing" && 0); }
void EmuGB::OpCBAF() { assert("Missing" && 0); }

void EmuGB::OpCBB0() { assert("Missing" && 0); }
void EmuGB::OpCBB1() { assert("Missing" && 0); }
void EmuGB::OpCBB2() { assert("Missing" && 0); }
void EmuGB::OpCBB3() { assert("Missing" && 0); }
void EmuGB::OpCBB4() { assert("Missing" && 0); }
void EmuGB::OpCBB5() { assert("Missing" && 0); }
void EmuGB::OpCBB6() { assert("Missing" && 0); }
void EmuGB::OpCBB7() { assert("Missing" && 0); }
void EmuGB::OpCBB8() { assert("Missing" && 0); }
void EmuGB::OpCBB9() { assert("Missing" && 0); }
void EmuGB::OpCBBA() { assert("Missing" && 0); }
void EmuGB::OpCBBB() { assert("Missing" && 0); }
void EmuGB::OpCBBC() { assert("Missing" && 0); }
void EmuGB::OpCBBD() { assert("Missing" && 0); }
void EmuGB::OpCBBE() { assert("Missing" && 0); }
void EmuGB::OpCBBF() { assert("Missing" && 0); }

void EmuGB::OpCBC0() { assert("Missing" && 0); }
void EmuGB::OpCBC1() { assert("Missing" && 0); }
void EmuGB::OpCBC2() { assert("Missing" && 0); }
void EmuGB::OpCBC3() { assert("Missing" && 0); }
void EmuGB::OpCBC4() { assert("Missing" && 0); }
void EmuGB::OpCBC5() { assert("Missing" && 0); }
void EmuGB::OpCBC6() { assert("Missing" && 0); }
void EmuGB::OpCBC7() { assert("Missing" && 0); }
void EmuGB::OpCBC8() { assert("Missing" && 0); }
void EmuGB::OpCBC9() { assert("Missing" && 0); }
void EmuGB::OpCBCA() { assert("Missing" && 0); }
void EmuGB::OpCBCB() { assert("Missing" && 0); }
void EmuGB::OpCBCC() { assert("Missing" && 0); }
void EmuGB::OpCBCD() { assert("Missing" && 0); }
void EmuGB::OpCBCE() { assert("Missing" && 0); }
void EmuGB::OpCBCF() { assert("Missing" && 0); }

void EmuGB::OpCBD0() { assert("Missing" && 0); }
void EmuGB::OpCBD1() { assert("Missing" && 0); }
void EmuGB::OpCBD2() { assert("Missing" && 0); }
void EmuGB::OpCBD3() { assert("Missing" && 0); }
void EmuGB::OpCBD4() { assert("Missing" && 0); }
void EmuGB::OpCBD5() { assert("Missing" && 0); }
void EmuGB::OpCBD6() { assert("Missing" && 0); }
void EmuGB::OpCBD7() { assert("Missing" && 0); }
void EmuGB::OpCBD8() { assert("Missing" && 0); }
void EmuGB::OpCBD9() { assert("Missing" && 0); }
void EmuGB::OpCBDA() { assert("Missing" && 0); }
void EmuGB::OpCBDB() { assert("Missing" && 0); }
void EmuGB::OpCBDC() { assert("Missing" && 0); }
void EmuGB::OpCBDD() { assert("Missing" && 0); }
void EmuGB::OpCBDE() { assert("Missing" && 0); }
void EmuGB::OpCBDF() { assert("Missing" && 0); }

void EmuGB::OpCBE0() { assert("Missing" && 0); }
void EmuGB::OpCBE1() { assert("Missing" && 0); }
void EmuGB::OpCBE2() { assert("Missing" && 0); }
void EmuGB::OpCBE3() { assert("Missing" && 0); }
void EmuGB::OpCBE4() { assert("Missing" && 0); }
void EmuGB::OpCBE5() { assert("Missing" && 0); }
void EmuGB::OpCBE6() { assert("Missing" && 0); }
void EmuGB::OpCBE7() { assert("Missing" && 0); }
void EmuGB::OpCBE8() { assert("Missing" && 0); }
void EmuGB::OpCBE9() { assert("Missing" && 0); }
void EmuGB::OpCBEA() { assert("Missing" && 0); }
void EmuGB::OpCBEB() { assert("Missing" && 0); }
void EmuGB::OpCBEC() { assert("Missing" && 0); }
void EmuGB::OpCBED() { assert("Missing" && 0); }
void EmuGB::OpCBEE() { assert("Missing" && 0); }
void EmuGB::OpCBEF() { assert("Missing" && 0); }

void EmuGB::OpCBF0() { assert("Missing" && 0); }
void EmuGB::OpCBF1() { assert("Missing" && 0); }
void EmuGB::OpCBF2() { assert("Missing" && 0); }
void EmuGB::OpCBF3() { assert("Missing" && 0); }
void EmuGB::OpCBF4() { assert("Missing" && 0); }
void EmuGB::OpCBF5() { assert("Missing" && 0); }
void EmuGB::OpCBF6() { assert("Missing" && 0); }
void EmuGB::OpCBF7() { assert("Missing" && 0); }
void EmuGB::OpCBF8() { assert("Missing" && 0); }
void EmuGB::OpCBF9() { assert("Missing" && 0); }
void EmuGB::OpCBFA() { assert("Missing" && 0); }
void EmuGB::OpCBFB() { assert("Missing" && 0); }
void EmuGB::OpCBFC() { assert("Missing" && 0); }
void EmuGB::OpCBFD() { assert("Missing" && 0); }
void EmuGB::OpCBFE() { assert("Missing" && 0); }
void EmuGB::OpCBFF() { assert("Missing" && 0); }