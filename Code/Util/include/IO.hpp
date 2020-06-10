#pragma once
#include "..\include\Definitions.hpp"

#include <type_traits>

ui8* LoadBinary(const char* path, unsigned int& file_size);

template<MemoryAccessType type, class T, class U, class V>
void ProcessMemory(T* pdata, U address, std::conditional_t<type == MemoryAccessType::Read, V&, V> data)
{
	static constexpr int dataSize = sizeof(V);

	if constexpr (type == MemoryAccessType::Read)
	{
		if constexpr (dataSize == 1) // Reading byte
		{
			data = pdata[address];
		}
		else if constexpr (dataSize == 2) // Reading Word
		{
			data = *reinterpret_cast<V*>(&pdata[address]);
		}
	}
	else if constexpr (type == MemoryAccessType::Write)
	{

	}
}