#include <Core.hpp>

#include <memory>

#include <Renderer.hpp>
#include <Window.hpp>
#include <UI.hpp>
#include <GB.hpp>
#include <EmulationManager.hpp>

#define IMGUI_DEFINE_MATH_OPERATORS 1

Core::Core( EmuRender* renderer, EmuWindow* window, EmuUI* ui ) : pRenderer( renderer ), pWindow( window ), pUI( ui )
{
	InitWindows();




	pUI->AddMenuItem( {"Emulator","Start-Windowed"}, "Emulator-Start" );
	pUI->AddMenuItem( {"Emulator","Stop"}, "Emulator-Stop" );


	pUI->AddMenuItem( {"File","Exit"}, "EXIT" );
}

Core::~Core()
{
	pVisualisation.reset();
	pEmulationManager.reset();
}

void Core::Update()
{
	pUI->StartRender();
	pUI->RenderMainMenuBar();
	pUI->RenderWindows();
	pUI->StopRender();
	pRenderer->Render();
	pWindow->Poll();

	UpdateTriggers();

	if (pEmulationManager != nullptr && pVisualisation != nullptr)
	{
		pEmulationManager->SyncEmulator( pVisualisation.get() );
	}
}

void GameVisualisation()
{
	ImVec2 windowSize = ImGui::GetWindowSize();
	EmuUI::GetInstance()->DrawScalingImage( 2, 166, 144, windowSize.x, windowSize.y );
}


void DrawBar( unsigned int width, float codeBarSizeScale, float dataBarSizeScale )
{
	const ImU32 codeColor = ImGui::ColorConvertFloat4ToU32( ImVec4( 0.464f, 0.400f, 0.978f, 1.000f ) );
	const ImU32 dataColor = ImGui::ColorConvertFloat4ToU32( ImVec4( 0.363f, 0.967f, 0.537f, 1.000f ) );

	unsigned int barHeight = 20;
	unsigned int barPadding = 4;

	ImGuiStyle& style = ImGui::GetStyle();
	ImVec2 cursorPos = ImGui::GetCursorScreenPos();
	ImVec4* colors = ImGui::GetStyle().Colors;

	ImVec2 dummySize = ImVec2( width - (style.WindowPadding.x * 2), barHeight );
	ImVec2 backgroundRenderSize = ImVec2( dummySize.x + cursorPos.x, dummySize.y + cursorPos.y );

	ImVec2 barPos = ImVec2( cursorPos.x + barPadding, cursorPos.y + barPadding );

	ImVec2 barRenderSize = ImVec2(
		(dummySize.x - (barPadding * 2)),
		(dummySize.y - (barPadding * 2))
	);

	ImVec2 barBackgroundRenderSize = ImVec2(
		barPos.x + barRenderSize.x,
		barPos.y + barRenderSize.y
	);

	ImVec2 codeBarRenderSize = ImVec2(
		barPos.x + (barRenderSize.x * codeBarSizeScale),
		barPos.y + barRenderSize.y
	);

	ImVec2 dataBarRenderSize = ImVec2(
		barPos.x + (barRenderSize.x * (dataBarSizeScale + codeBarSizeScale)),
		barPos.y + barRenderSize.y
	);


	
	// Background
	{
		ImGui::GetWindowDrawList()->AddRectFilled( cursorPos, backgroundRenderSize, ImGui::ColorConvertFloat4ToU32( colors[ImGuiCol_FrameBg] ) );
	}
	// Background 2
	{
		ImGui::GetWindowDrawList()->AddRectFilled( barPos, barBackgroundRenderSize, ImGui::ColorConvertFloat4ToU32( ImVec4( 0.6f, 0.6f, 0.6f, 1.0f) ) );
	}
	// Data
	{
		ImGui::GetWindowDrawList()->AddRectFilled( barPos, dataBarRenderSize, dataColor );
	}
	// Code
	{
		ImGui::GetWindowDrawList()->AddRectFilled( barPos, codeBarRenderSize, codeColor );
	}
	
	ImGui::Dummy( dummySize );
}

void MemoryAllocationVisualisation()
{

	ImGui::Button("Test");


	ImVec2 windowSize = ImGui::GetWindowSize();


	DrawBar( windowSize.x / 2, 0.4f, 0.3f );

	ImGui::SameLine();

	DrawBar( windowSize.x / 2, 0.2f, 0.5f );
}


void PixelPlotter()
{

	ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 1, 1 ) );

	ImGuiStyle& style = ImGui::GetStyle();
	ImVec2 dummySize = ImVec2( 20, 20 );
	for (int y = 0; y < 8; y++)
	{
		for (int x = 0; x < 8; x++)
		{
			ImVec2 cursorPos = ImGui::GetCursorScreenPos();
			ImVec2 backgroundRenderSize = ImVec2( dummySize.x + cursorPos.x, dummySize.y + cursorPos.y );


			ImGui::GetWindowDrawList()->AddRectFilled( cursorPos, backgroundRenderSize, ImGui::ColorConvertFloat4ToU32( ImVec4( 0.6f, 0.6f, 0.6f, 1.0f ) ) );


			ImGui::Dummy( dummySize );

			if (EmuUI::ElementClicked())
			{

			}

			ImGui::SameLine();
		}
		ImGui::NewLine();
	}

	ImGui::PopStyleVar();
}


void Core::InitWindows()
{
	pUI->RegisterWindow( new EmuUI::Window( "Game", ImGuiWindowFlags_NoCollapse, GameVisualisation, true ) );
	pUI->RegisterWindow( new EmuUI::Window( "MemoryAllocation", ImGuiWindowFlags_NoCollapse, MemoryAllocationVisualisation, true ) );
	pUI->RegisterWindow( new EmuUI::Window( "PixelPlotter", ImGuiWindowFlags_NoCollapse, PixelPlotter, true ) );
}

void Core::UpdateTriggers()
{
	if (pUI->IsSelectedElement( "Emulator-Start" ))
	{
		if (pEmulationManager == nullptr)
		{
			pEmulationManager = std::make_unique<EmulationManager>( "Games/GB/Pocket.gb" );

			pEmulationManager->WaitTillReady();

			pVisualisation = std::make_unique<Visualisation>( pEmulationManager->GetScreenWidth(), pEmulationManager->GetScreenHeight() );
		}
	}
	

	if (pUI->IsSelectedElement( "Emulator-Stop" ))
	{
		pEmulationManager->Stop();
		pEmulationManager.reset();
		pVisualisation->ClearScreen();
		pVisualisation.reset();
	}


	if (pUI->IsSelectedElement( "EXIT" ))
	{
		pWindow->CloseWindow();
	}
}
