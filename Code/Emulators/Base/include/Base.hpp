#pragma once

template <typename Impl>
struct EmuBase
{
	enum class ConsoleKeys
	{
		RIGHT = 0,
		LEFT = 1,
		UP = 2,
		DOWN = 3,
		A = 4,
		B = 5,
		SELECT = 6,
		START = 7,
	};

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
private:
	Impl* impl() {
		return static_cast<Impl*>(this);
	}

};