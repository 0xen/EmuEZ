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
private:
	Impl* impl() {
		return static_cast<Impl*>(this);
	}

};