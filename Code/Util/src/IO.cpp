#include "..\include\IO.hpp"

#include <fstream>

ui8* LoadBinary(const char* path, unsigned int& file_size)
{
	// Define we are loading the file in binary mode
	std::ifstream file(path, std::ios::in | std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		file_size = 0;
		return nullptr;
	}
	file_size = static_cast<int> (file.tellg());

	// Allocate new dynamic memory for the system
	ui8* dynamic_memory = new ui8[file_size];
	file.seekg(0, std::ios::beg);
	file.read((char*)dynamic_memory, file_size);
	file.close();

	return dynamic_memory;
}
