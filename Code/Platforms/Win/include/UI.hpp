#pragma once

#include <Renderer.hpp>
#include <imgui.h>
#include <vector>

class EmuUI
{
	struct Window
	{
		Window(const char* Name, ImGuiWindowFlags Flags, std::function<void()>	FPtr, bool Open = true) : name(Name), fPtr(FPtr), flags(Flags), open(Open) {}
		const char*				name;
		ImGuiWindowFlags		flags;
		std::function<void()>	fPtr;
		bool					open = false;
	};
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
	

	EmuUI(EmuRender* renderer, EmuWindow* window);
	~EmuUI();

	void StartRender();

	void RenderMainMenuBar();

	void RenderWindows();

	void StopRender();

	void ImGuiCommandBufferCallback(VkCommandBuffer& command_buffer);

	void RegisterWindow(Window* window);

private:
	void InitWindows();

	void InitImGui();

	void InitImGUIBuffers();
	void DeInitImGUIBuffers();

	void InitImGuiDescriptors();
	void DeInitImGuiDescriptors();

	void InitPipeline();
	void DeInitPipeline();

	void ResetIndirectDrawBuffer();
	void DockSpace();


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


	VkDescriptorPool imgui_gpu_context_descriptor_pool;
	VkDescriptorSetLayout imgui_gpu_context_descriptor_set_layout;
	VkDescriptorSet imgui_gpu_context_descriptor_set;
	ImGUIGpuContext imgui_gpu_context;
	EmuRender::SBuffer imgui_gpu_context_buffer;


	std::vector<ImGUIDrawInstance> imgui_draw_instances;

	std::vector<Window*> m_windows;

	EmuRender* pRenderer;
	EmuWindow* pWindow;
};