#pragma once

#include <memory>
#include <vector>

#include <Visualisation.hpp>
#include <EmulationManager.hpp>

#include <pugixml.hpp>

class EmuRender;
class EmuWindow;
class EmuUI;

class Core
{
public:

	Core( EmuRender* renderer, EmuWindow* window, EmuUI* ui );

	~Core();

	void Update();

	bool StartEmulator( EGame game );

	bool IsEmulatorRunning();

	std::vector<EGame>& GetGames();

	void AddGame( const char* path );

	void ScanFolder( const char* path );

	void SaveConfig();

	void LoadConfig();

	static Core* GetInstance();

private:

	void InitWindows();

	void UpdateTriggers();

	static Core* mInstance;

	std::vector<EGame> mGames;

	EmuRender* pRenderer;
	EmuWindow* pWindow;
	EmuUI* pUI;
	std::unique_ptr<Visualisation> pVisualisation;
	std::unique_ptr<EmulationManager> pEmulationManager;
};