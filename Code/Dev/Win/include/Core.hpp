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

private:

	void InitWindows();

	void UpdateTriggers();

	EmuRender* pRenderer;
	EmuWindow* pWindow;
	EmuUI* pUI;
	std::unique_ptr<Visualisation> pVisualisation;
	std::unique_ptr<EmulationManager> pEmulationManager;
};