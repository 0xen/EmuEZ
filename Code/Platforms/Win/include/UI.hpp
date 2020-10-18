#pragma once

#include <Renderer.hpp>
#include <imgui.h>
#include <vector>
#include <map>

class EmuUI
{
	struct ImGUIDrawInstance
	{
		VkRect2D scissor;
	};
	struct ImGUIGpuContext
	{
		float width;
		float height;
	};
public:
	struct Window
	{
		Window( const char* Name, ImGuiWindowFlags Flags, std::function<void()>	FPtr, bool Open = true ) : name( Name ), fPtr( FPtr ), flags( Flags ), open( Open ) {}
		const char* name;
		ImGuiWindowFlags		flags;
		std::function<void()>	fPtr;
		bool					open = false;
	};
	struct MenuItem
	{
		std::string selected;
		std::map<std::string, MenuItem> children;
	};
	

	EmuUI(EmuRender* renderer, EmuWindow* window);
	~EmuUI();

	void StartRender();

	void RenderMainMenuBar();

	void RenderWindows();

	void StopRender();

	void ImGuiCommandBufferCallback( VkCommandBuffer& command_buffer );

	void RegisterWindow( Window* window );

	bool IsSelectedElement( std::string name );

	void MarkSelectedElement( std::string name );

	void AddMenuItem( std::vector<std::string> path, std::string selected );

	EmuRender::STexture& GetVisualisationTexture();

	bool IsWindowFocused(); 
	
	void DrawScalingImage( unsigned int texture_id, unsigned int image_width, unsigned int image_height, unsigned int window_width, unsigned int window_height );

	unsigned int GetMenuBarHeight();

	static EmuUI* GetInstance();
private:

	friend class Visualisation;

	void InitImGui();

	void InitImGUIBuffers();
	void DeInitImGUIBuffers();

	void InitImGuiDescriptors();
	void DeInitImGuiDescriptors();

	void InitPipeline();
	void DeInitPipeline();

	void ResetIndirectDrawBuffer();
	void DockSpace();

	bool ElementClicked();

	void CalculateImageScaling( unsigned int image_width, unsigned int image_height, unsigned int window_width, unsigned int window_height, ImVec2& new_image_offset, ImVec2& new_image_size );

	void RenderMainMenuItem(std::string text, MenuItem* item);

	static EmuUI* instance;

	EmuRender::SGraphicsPipeline imgui_pipeline;

	EmuRender::SBuffer imgui_vertex_buffer;
	ImDrawVert* imgui_vertex_data = nullptr;

	EmuRender::SBuffer imgui_index_buffer;
	uint32_t* imgui_index_data = nullptr;

	EmuRender::SBuffer imgui_texture_buffer;
	int* imgui_texture_data = nullptr;

	unsigned int indirect_draw_buffer_size;
	EmuRender::SBuffer indexed_indirect_buffer;
	VkDrawIndexedIndirectCommand* m_indexed_indirect_command;


	VkDescriptorPool imgui_texture_descriptor_pool;
	VkDescriptorSetLayout imgui_texture_descriptor_set_layout;
	VkDescriptorSet imgui_texture_descriptor_set;

	EmuRender::SBuffer imgui_font_texture_buffer;
	EmuRender::STexture imgui_font_texture;

	EmuRender::SBuffer visualisation_texture_buffer;
	EmuRender::STexture visualisation_texture;


	VkDescriptorPool imgui_gpu_context_descriptor_pool;
	VkDescriptorSetLayout imgui_gpu_context_descriptor_set_layout;
	VkDescriptorSet imgui_gpu_context_descriptor_set;
	ImGUIGpuContext imgui_gpu_context;
	EmuRender::SBuffer imgui_gpu_context_buffer;


	std::vector<ImGUIDrawInstance> imgui_draw_instances;

	std::vector<Window*> m_windows;

	std::map<std::string, MenuItem> m_menu_items;

	std::map<std::string, bool> m_selected_element;

	EmuRender* pRenderer;
	EmuWindow* pWindow;
};