#pragma once

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
private:
	Impl* impl() {
		return static_cast<Impl*>(this);
	}

};