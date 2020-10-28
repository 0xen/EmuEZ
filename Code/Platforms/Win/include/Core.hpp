#pragma once

#include <memory>

#include <Visualisation.hpp>
#include <EmulationManager.hpp>

class EmuRender;
class EmuWindow;
class EmuUI;

class Core
{
public:

	Core( EmuRender* renderer, EmuWindow* window, EmuUI* ui );

	~Core();

	void Update();

	bool StartEmulator( EEmulator emulator, const char* path );

	bool IsEmulatorRunning();

	static Core* GetInstance();

private:

	void InitWindows();

	void UpdateTriggers();

	static Core* mInstance;

	EmuRender* pRenderer;
	EmuWindow* pWindow;
	EmuUI* pUI;
	std::unique_ptr<Visualisation> pVisualisation;
	std::unique_ptr<EmulationManager> pEmulationManager;
};