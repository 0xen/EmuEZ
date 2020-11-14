#pragma once

#ifdef __linux__
#define __forceinline inline
#endif

#include <fstream>

enum class ConsoleKeys
{
	RIGHT,
	LEFT,
	UP,
	DOWN,
	A,
	B,
	SELECT,
	START,


	SAVE_STATE,
	LOAD_STATE
};

enum class SaveType
{
	PowerDown,	// Batter/memory card saving
	SaveState	// Save at any point during the running process
};

template <typename Impl>
struct EmuBase
{

	bool Init(const char* path)
	{
		return impl()->InitEmu(path);
	}
	void Tick()
	{
		impl()->TickEmu();
	}
	__forceinline unsigned int ScreenWidth()
	{
		return impl()->ScreenWidthEmu();
	}
	__forceinline unsigned int ScreenHeight()
	{
		return impl()->ScreenHeightEmu();
	}
	void GetScreenBuffer(char*& ptr, unsigned int& size)
	{
		impl()->GetScreenBufferEmu(ptr, size);
	}

	void KeyPress(ConsoleKeys key)
	{
		impl()->KeyPressEmu(key);
	}

	void KeyRelease(ConsoleKeys key)
	{
		impl()->KeyReleaseEmu(key);
	}

	bool IsKeyDown(ConsoleKeys key)
	{
		return impl()->IsKeyDownEmu(key);
	}
	void SkipBIOS()
	{
		impl()->SkipBIOSEmu();
	}
	void Save( SaveType type, std::ostream& stream )
	{
		impl( )->SaveEmu( type, stream );
	}
	void Load( SaveType type, std::istream& stream )
	{
		impl( )->LoadEmu( type, stream );
	}
private:
	Impl* impl() {
		return static_cast<Impl*>(this);
	}

};