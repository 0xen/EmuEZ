#pragma once

#include <memory>
#include <vector>

#include <Visualisation.hpp>
#include <EmulationManager.hpp>

#include <pugixml.hpp>
#include <base.hpp>

class EmuRender;
class EmuWindow;
class EmuUI;


class Core
{
public:

	enum EView
	{
		Dashboard = 0,
		Emulator = 1
	};
	struct KeyInstance
	{
		ConsoleKeys key; // Key to be passed to the emulator

		int index;

		int startRange;
	};
	enum EInputType
	{
		Keyboard,
		JoyHat,
		JoyButton,
		JoyAxis
	};

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

	std::map<EView, std::map<EInputType, std::vector<KeyInstance>>>& GetKeyMappings( );
private:

	void Save( pugi::xml_node& node );

	void Load( pugi::xml_node& node );

	void InitWindows();

	void UpdateTriggers();

	static Core* mInstance;

	std::vector<EGame> mGames;

	std::map<EView, std::map<EInputType, std::vector<KeyInstance>>> mKeyMappings;

	EmuRender* pRenderer;
	EmuWindow* pWindow;
	EmuUI* pUI;
	std::unique_ptr<Visualisation> pVisualisation;
	std::unique_ptr<EmulationManager> pEmulationManager;
};