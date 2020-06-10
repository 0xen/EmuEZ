#pragma once
// 1 Byte - 8 Bit
typedef unsigned __int8 ui8;
typedef signed __int8 i8;

// 2 Byte - 16 Bit
typedef unsigned __int16 ui16;
typedef signed __int16 i16;

// 4 Byte - 32 Bit
typedef unsigned __int32 ui32;
typedef signed __int32 i32;

// 8 Byte - 64 Bit
typedef unsigned __int64 ui64;
typedef signed __int64 i64;

enum class MemoryAccessType : ui32
{
	Read,
	Write
};

enum class MemoryType : ui32
{
	ui8,
	i8
};