#include <UI.hpp>
#include <Window.hpp>

#include <SDL.h>
#include <SDL_syswm.h>

#define MAX_VERTICIES 100000
#define MAX_INDICIES 100000

EmuUI* EmuUI::instance = nullptr;

void WindowPoll( SDL_Event& event)
{
	ImGuiIO& io = ImGui::GetIO();

	switch (event.type)
	{
		case SDL_WINDOWEVENT:
			switch (event.window.event)
			{
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					io.DisplaySize = ImVec2( event.window.data1, event.window.data2 );
					break;
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			if (event.button.button == SDL_BUTTON_LEFT) io.MouseDown[0] = event.button.state == SDL_PRESSED;
			if (event.button.button == SDL_BUTTON_RIGHT) io.MouseDown[1] = event.button.state == SDL_PRESSED;
			if (event.button.button == SDL_BUTTON_MIDDLE) io.MouseDown[2] = event.button.state == SDL_PRESSED;
			break;

		case SDL_MOUSEMOTION:
		{
			io.MousePos = ImVec2( event.motion.x, event.motion.y );
		}
		break;
		case SDL_TEXTINPUT:
		{
			io.AddInputCharactersUTF8( event.text.text );
			break;
		}
		case SDL_MOUSEWHEEL:
		{
			if (event.wheel.x > 0) io.MouseWheelH += 1;
			if (event.wheel.x < 0) io.MouseWheelH -= 1;
			if (event.wheel.y > 0) io.MouseWheel += 1;
			if (event.wheel.y < 0) io.MouseWheel -= 1;
			break;
		}
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		{
			int key = event.key.keysym.scancode;
			IM_ASSERT( key >= 0 && key < IM_ARRAYSIZE( io.KeysDown ) );
			{
				io.KeysDown[key] = (event.type == SDL_KEYDOWN);
			}
			io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
			io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
			io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
			io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);

		}
		break;
	}
}

EmuUI::EmuUI(EmuRender* renderer, EmuWindow* window) : pRenderer(renderer), pWindow(window)
{
	instance = this;
	window->RegisterWindowPoll( WindowPoll );
	InitImGui();
	InitImGUIBuffers();
	InitImGuiDescriptors();
	InitPipeline();
}

EmuUI::~EmuUI()
{
	DeInitImGUIBuffers();
	DeInitImGuiDescriptors();
	DeInitPipeline();
	for (auto& w : m_windows)
	{
		delete w;
	}
}

void EmuUI::StartRender()
{
	ImGui::NewFrame();
	ImGui::ShowDemoWindow();
	DockSpace();
}

void EmuUI::RenderMainMenuBar()
{
	if (ImGui::BeginMainMenuBar())
	{

		for (auto it = m_menu_items.begin(); it != m_menu_items.end(); ++it)
		{
			RenderMainMenuItem( it->first, &it->second );
		}

		ImGui::EndMainMenuBar();
	}
}

void HexEditor()
{


}

void EmuUI::RenderWindows()
{

	bool open = true;

	HexEditor();

	for (auto& w : m_windows)
	{
		ImGui::Begin(w->name, &open, ImGuiWindowFlags_NoCollapse);

		w->fPtr();

		ImGui::End();
	}

}

void EmuUI::StopRender()
{
	ImGui::Render();

	ImDrawData* imDrawData = ImGui::GetDrawData();

	ImVec2 clip_off = imDrawData->DisplayPos;         // (0,0) unless using multi-viewports
	ImVec2 clip_scale = imDrawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)
	int fb_width = (int)(imDrawData->DisplaySize.x * imDrawData->FramebufferScale.x);
	int fb_height = (int)(imDrawData->DisplaySize.y * imDrawData->FramebufferScale.y);

	ImDrawVert* temp_vertex_data = imgui_vertex_data;
	unsigned int* temp_index_data = imgui_index_data;
	int* temp_texture_data = imgui_texture_data;
	unsigned int index_count = 0;
	unsigned int vertex_count = 0;
	int drawGroup = 0;

	ResetIndirectDrawBuffer();

	unsigned int last_draw_instance_count = imgui_draw_instances.size();

	for (int n = 0; n < imDrawData->CmdListsCount; n++)
	{

		const ImDrawList* cmd_list = imDrawData->CmdLists[n];

		memcpy(temp_vertex_data, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));

		// Loop through and manually add a offset to the index's so they can all be rendered in one render pass
		for (int i = 0; i < cmd_list->VtxBuffer.Size; i++)
		{
			temp_vertex_data[i] = cmd_list->VtxBuffer.Data[i];
		}
		for (int i = 0; i < cmd_list->IdxBuffer.Size; i++)
		{
			temp_index_data[i] = cmd_list->IdxBuffer.Data[i];
		}

		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];



			// Generate the clip area
			ImVec4 clip_rect;
			clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
			clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
			clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
			clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

			// If the object is out of the scissor, ignore it
			if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
			{
				// Negative offsets are illegal for vkCmdSetScissor
				if (clip_rect.x < 0.0f)
					clip_rect.x = 0.0f;
				if (clip_rect.y < 0.0f)
					clip_rect.y = 0.0f;

				// Generate a vulkan friendly scissor area
				VkRect2D scissor;
				scissor.offset.x = (int32_t)(clip_rect.x);
				scissor.offset.y = (int32_t)(clip_rect.y);
				scissor.extent.width = (uint32_t)(clip_rect.z - clip_rect.x);
				scissor.extent.height = (uint32_t)(clip_rect.w - clip_rect.y);

				if (drawGroup >= imgui_draw_instances.size())
				{
					imgui_draw_instances.push_back({});
				}

				imgui_draw_instances[drawGroup].scissor = scissor;

				VkDrawIndexedIndirectCommand& indirect_command = m_indexed_indirect_command[drawGroup];
				indirect_command.indexCount = pcmd->ElemCount;
				indirect_command.instanceCount = 1;
				indirect_command.firstIndex = pcmd->IdxOffset + index_count;
				indirect_command.vertexOffset = pcmd->VtxOffset + vertex_count;
				indirect_command.firstInstance = 0;

				temp_texture_data[drawGroup] = (int)pcmd->TextureId;

				drawGroup++;
			}
		}

		temp_vertex_data += cmd_list->VtxBuffer.Size;
		temp_index_data += cmd_list->IdxBuffer.Size;

		vertex_count += cmd_list->VtxBuffer.Size;
		index_count += cmd_list->IdxBuffer.Size;
	}
	if (last_draw_instance_count != drawGroup)
	{
		pRenderer->RequestCommandBufferRebuild();
		if (last_draw_instance_count > drawGroup)
		{
			imgui_draw_instances.resize(drawGroup);
		}

	}


	bool b = (vertex_count < MAX_VERTICIES && index_count < MAX_INDICIES);
	assert(b && "Array Limit reached");

	imgui_gpu_context.width = pWindow->GetWidth();
	imgui_gpu_context.height = pWindow->GetHeight();
	pRenderer->TransferToGPUBuffer(imgui_gpu_context_buffer, &imgui_gpu_context, imgui_gpu_context_buffer.buffer_size);

	pRenderer->TransferToGPUBuffer(imgui_vertex_buffer, imgui_vertex_data, imgui_vertex_buffer.buffer_size);

	pRenderer->TransferToGPUBuffer(imgui_index_buffer, imgui_index_data, imgui_index_buffer.buffer_size);

	pRenderer->TransferToGPUBuffer(imgui_texture_buffer, imgui_texture_data, imgui_texture_buffer.buffer_size);

	pRenderer->TransferToGPUBuffer(indexed_indirect_buffer, m_indexed_indirect_command, indexed_indirect_buffer.buffer_size);

	// Need to find a solution for the scisor needing to be regenerated every frame

	pRenderer->RequestCommandBufferRebuild();
}


void EmuUI::InitImGUIBuffers()
{
	{
		const unsigned int vertex_buffer_size = MAX_VERTICIES;

		imgui_vertex_data = new ImDrawVert[vertex_buffer_size];

		imgui_vertex_buffer.buffer_size = sizeof(ImDrawVert) * vertex_buffer_size;
		imgui_vertex_buffer.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		imgui_vertex_buffer.sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
		imgui_vertex_buffer.buffer_memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		pRenderer->CreateBuffer(imgui_vertex_buffer);
		pRenderer->MapMemory(imgui_vertex_buffer);
	}
	{
		const unsigned int index_buffer_size = MAX_INDICIES;

		imgui_index_data = new uint32_t[index_buffer_size];

		imgui_index_buffer.buffer_size = sizeof(uint32_t) * index_buffer_size;
		imgui_index_buffer.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		imgui_index_buffer.sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
		imgui_index_buffer.buffer_memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		pRenderer->CreateBuffer(imgui_index_buffer);
		pRenderer->MapMemory(imgui_index_buffer);
	}
	{
		const unsigned int texture_buffer_size = 1000;

		imgui_texture_data = new int[texture_buffer_size];

		imgui_texture_buffer.buffer_size = sizeof(int) * texture_buffer_size;
		imgui_texture_buffer.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		imgui_texture_buffer.sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
		imgui_texture_buffer.buffer_memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		pRenderer->CreateBuffer(imgui_texture_buffer);
		pRenderer->MapMemory(imgui_texture_buffer);
	}
	{
		indirect_draw_buffer_size = 1000;

		m_indexed_indirect_command = new VkDrawIndexedIndirectCommand[indirect_draw_buffer_size];

		indexed_indirect_buffer.buffer_size = sizeof(VkDrawIndexedIndirectCommand) * indirect_draw_buffer_size;
		indexed_indirect_buffer.usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		indexed_indirect_buffer.sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
		indexed_indirect_buffer.buffer_memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		pRenderer->CreateBuffer(indexed_indirect_buffer);
		pRenderer->MapMemory(indexed_indirect_buffer);
	}
	{
		imgui_gpu_context_buffer.buffer_size = sizeof(ImGUIGpuContext);
		imgui_gpu_context_buffer.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		imgui_gpu_context_buffer.sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
		imgui_gpu_context_buffer.buffer_memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		pRenderer->CreateBuffer(imgui_gpu_context_buffer);
		pRenderer->MapMemory(imgui_gpu_context_buffer);
	}
	{
		ImGuiIO& io = ImGui::GetIO();

		// Create font texture
		unsigned char* font_data;
		int font_width, font_height;
		io.Fonts->GetTexDataAsRGBA32( &font_data, &font_width, &font_height );


		const unsigned int texture_size = font_width * font_height * 4;

		imgui_font_texture_buffer.buffer_size = texture_size;
		imgui_font_texture_buffer.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		imgui_font_texture_buffer.sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
		imgui_font_texture_buffer.buffer_memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		pRenderer->CreateBuffer( imgui_font_texture_buffer );
		pRenderer->MapMemory( imgui_font_texture_buffer );
		pRenderer->TransferToGPUBuffer( imgui_font_texture_buffer, font_data, texture_size );


		imgui_font_texture.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imgui_font_texture.format = VK_FORMAT_R8G8B8A8_UNORM;
		imgui_font_texture.width = font_width;
		imgui_font_texture.height = font_height;

		pRenderer->CreateTexture( imgui_font_texture );
		pRenderer->TransferToGPUTexture( imgui_font_texture_buffer, imgui_font_texture );
		pRenderer->CreateSampler( imgui_font_texture );


		io.Fonts->TexID = (ImTextureID)1;

		delete[] font_data;
	}

	{
		const unsigned int texture_size = 160 * 144 * 4;
		char* texture_data = new char[texture_size];

		for (int i = 0; i < texture_size; i++)
		{
			texture_data[i] = 0;
		}

		visualisation_texture_buffer.buffer_size = texture_size;
		visualisation_texture_buffer.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		visualisation_texture_buffer.sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
		visualisation_texture_buffer.buffer_memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		pRenderer->CreateBuffer( visualisation_texture_buffer );
		pRenderer->MapMemory( visualisation_texture_buffer );
		pRenderer->TransferToGPUBuffer( visualisation_texture_buffer, texture_data, texture_size );


		visualisation_texture.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		visualisation_texture.format = VK_FORMAT_R8G8B8A8_UNORM;
		visualisation_texture.width = 160;
		visualisation_texture.height = 144;

		pRenderer->CreateTexture( visualisation_texture );
		pRenderer->TransferToGPUTexture( visualisation_texture_buffer, visualisation_texture );
		pRenderer->CreateSampler( visualisation_texture );



		delete[] texture_data;

	}
}

void EmuUI::DeInitImGUIBuffers()
{
	pRenderer->DestroyBuffer( visualisation_texture_buffer );
	pRenderer->DestroyBuffer( imgui_texture_buffer );
	pRenderer->DestroyBuffer( imgui_index_buffer );
	pRenderer->DestroyBuffer( imgui_vertex_buffer );
}

void EmuUI::InitImGuiDescriptors()
{
	const uint32_t descriptor_pool_size_count = 1;

	{	// Texture pool and set
		VkDescriptorPoolSize texture_pool_size[descriptor_pool_size_count] = {
			VkHelper::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10)
		};

		pRenderer->CreateDescriptorPool(imgui_texture_descriptor_pool, texture_pool_size, descriptor_pool_size_count, 10);


		VkDescriptorSetLayoutBinding layout_bindings[descriptor_pool_size_count] = {
			VkHelper::DescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10, VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		pRenderer->CreateDescriptorSetLayout(imgui_texture_descriptor_set_layout, layout_bindings, descriptor_pool_size_count);

		pRenderer->AllocateDescriptorSet(imgui_texture_descriptor_set, imgui_texture_descriptor_pool, imgui_texture_descriptor_set_layout, 1);
	}

	{	// GPU Data pool and set
		VkDescriptorPoolSize pool_size[descriptor_pool_size_count] = {
			VkHelper::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
		};

		pRenderer->CreateDescriptorPool(imgui_gpu_context_descriptor_pool, pool_size, descriptor_pool_size_count, 1);


		VkDescriptorSetLayoutBinding layout_bindings[descriptor_pool_size_count] = {
			VkHelper::DescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT)
		};

		pRenderer->CreateDescriptorSetLayout(imgui_gpu_context_descriptor_set_layout, layout_bindings, descriptor_pool_size_count);

		pRenderer->AllocateDescriptorSet(imgui_gpu_context_descriptor_set, imgui_gpu_context_descriptor_pool, imgui_gpu_context_descriptor_set_layout, 1);
	}

	{ // Textures
		std::vector<VkDescriptorImageInfo> descriptorImageInfos;
		// Font Texture
		{
			VkDescriptorImageInfo info;
			info.sampler = imgui_font_texture.sampler;
			info.imageView = imgui_font_texture.view;
			info.imageLayout = imgui_font_texture.layout;
			descriptorImageInfos.push_back( info );
		}
		// Visualisation Texture
		{
			VkDescriptorImageInfo info;
			info.sampler = visualisation_texture.sampler;
			info.imageView = visualisation_texture.view;
			info.imageLayout = visualisation_texture.layout;
			descriptorImageInfos.push_back( info );
		}


		{ // Update the Descriptor Set
			VkWriteDescriptorSet descriptorWrite = {};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = imgui_texture_descriptor_set;
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrite.descriptorCount = static_cast<uint32_t>(descriptorImageInfos.size());
			descriptorWrite.pBufferInfo = VK_NULL_HANDLE;
			descriptorWrite.pImageInfo = VK_NULL_HANDLE;
			descriptorWrite.pTexelBufferView = VK_NULL_HANDLE;
			descriptorWrite.pNext = VK_NULL_HANDLE;

			static const int offset = offsetof(VkWriteDescriptorSet, pImageInfo);

			VkDescriptorImageInfo** data = reinterpret_cast<VkDescriptorImageInfo**>(reinterpret_cast<uint8_t*>(&descriptorWrite) + offset);
			*data = descriptorImageInfos.data();



			vkUpdateDescriptorSets(
				pRenderer->GetDevice(),
				1,
				&descriptorWrite,
				0,
				NULL
			);
		}
	}

	{ // GPU contexts
		std::vector<VkDescriptorBufferInfo> descriptorInfos;
		{
			VkDescriptorBufferInfo info;
			info.buffer = imgui_gpu_context_buffer.buffer;
			info.offset = 0;
			info.range = sizeof(ImGUIGpuContext);
			descriptorInfos.push_back(info);
		}


		{ // Update the Descriptor Set
			VkWriteDescriptorSet descriptorWrite = {};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = imgui_gpu_context_descriptor_set;
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = static_cast<uint32_t>(descriptorInfos.size());
			descriptorWrite.pBufferInfo = descriptorInfos.data();
			descriptorWrite.pImageInfo = VK_NULL_HANDLE;
			descriptorWrite.pTexelBufferView = VK_NULL_HANDLE;
			descriptorWrite.pNext = VK_NULL_HANDLE;


			vkUpdateDescriptorSets(
				pRenderer->GetDevice(),
				1,
				&descriptorWrite,
				0,
				NULL
			);
		}
	}

	
}

void EmuUI::DeInitImGuiDescriptors()
{
}

void EmuUI::InitPipeline()
{
	const uint32_t shader_count = 2;

	const char* shader_paths[shader_count]{
		"Shaders/ImGUI/vert.spv",
		"Shaders/ImGUI/frag.spv"
	};
	VkShaderStageFlagBits shader_stages_bits[shader_count]{
		VK_SHADER_STAGE_VERTEX_BIT,
		VK_SHADER_STAGE_FRAGMENT_BIT
	};

	const uint32_t descriptor_set_layout_count = 2;

	const uint32_t vertex_input_attribute_description_count = 4;

	const uint32_t vertex_input_binding_description_count = 2;

	const uint32_t dynamic_state_count = 3;


	VkDynamicState dynamic_states[dynamic_state_count] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	imgui_pipeline.shader_count = shader_count;
	imgui_pipeline.shader_paths = shader_paths;
	imgui_pipeline.shader_stages_bits = shader_stages_bits;
	imgui_pipeline.shader_modules = std::unique_ptr<VkShaderModule>(new VkShaderModule[shader_count]);
	imgui_pipeline.descriptor_set_layout_count = descriptor_set_layout_count;
	imgui_pipeline.descriptor_set_layout = std::unique_ptr<VkDescriptorSetLayout>(new VkDescriptorSetLayout[descriptor_set_layout_count]);;

	{
		imgui_pipeline.descriptor_set_layout.get()[0] = imgui_texture_descriptor_set_layout;
		imgui_pipeline.descriptor_set_layout.get()[1] = imgui_gpu_context_descriptor_set_layout;
	}

	imgui_pipeline.vertex_input_attribute_description_count = vertex_input_attribute_description_count;
	imgui_pipeline.vertex_input_attribute_descriptions = std::unique_ptr<VkVertexInputAttributeDescription>(new VkVertexInputAttributeDescription[vertex_input_attribute_description_count]);
	{
		// Position
		imgui_pipeline.vertex_input_attribute_descriptions.get()[0].binding = 0;
		imgui_pipeline.vertex_input_attribute_descriptions.get()[0].location = 0;
		imgui_pipeline.vertex_input_attribute_descriptions.get()[0].format = VK_FORMAT_R32G32_SFLOAT;
		imgui_pipeline.vertex_input_attribute_descriptions.get()[0].offset = offsetof(ImDrawVert, pos);

		// UV
		imgui_pipeline.vertex_input_attribute_descriptions.get()[1].binding = 0;
		imgui_pipeline.vertex_input_attribute_descriptions.get()[1].location = 1;
		imgui_pipeline.vertex_input_attribute_descriptions.get()[1].format = VK_FORMAT_R32G32_SFLOAT;
		imgui_pipeline.vertex_input_attribute_descriptions.get()[1].offset = offsetof(ImDrawVert, uv);

		// Color
		imgui_pipeline.vertex_input_attribute_descriptions.get()[2].binding = 0;
		imgui_pipeline.vertex_input_attribute_descriptions.get()[2].location = 2;
		imgui_pipeline.vertex_input_attribute_descriptions.get()[2].format = VK_FORMAT_R8G8B8A8_UNORM;
		imgui_pipeline.vertex_input_attribute_descriptions.get()[2].offset = offsetof(ImDrawVert, col);

		// Texture ID
		imgui_pipeline.vertex_input_attribute_descriptions.get()[3].binding = 1;
		imgui_pipeline.vertex_input_attribute_descriptions.get()[3].location = 3;
		imgui_pipeline.vertex_input_attribute_descriptions.get()[3].format = VK_FORMAT_R32_SINT;
		imgui_pipeline.vertex_input_attribute_descriptions.get()[3].offset = 0;
	}
	imgui_pipeline.vertex_input_binding_description_count = vertex_input_binding_description_count;
	imgui_pipeline.vertex_input_binding_descriptions = std::unique_ptr<VkVertexInputBindingDescription>(new VkVertexInputBindingDescription[vertex_input_binding_description_count]);
	{
		// Position, UV, Color
		imgui_pipeline.vertex_input_binding_descriptions.get()[0].binding = 0;
		imgui_pipeline.vertex_input_binding_descriptions.get()[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		imgui_pipeline.vertex_input_binding_descriptions.get()[0].stride = sizeof(ImDrawVert);

		// Texture ID
		imgui_pipeline.vertex_input_binding_descriptions.get()[1].binding = 1;
		imgui_pipeline.vertex_input_binding_descriptions.get()[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
		imgui_pipeline.vertex_input_binding_descriptions.get()[1].stride = sizeof(int);
	}
	imgui_pipeline.dynamic_state_count = dynamic_state_count;
	imgui_pipeline.dynamic_states = dynamic_states;

	pRenderer->CreateGraphicsPipeline(imgui_pipeline);
}

void EmuUI::DeInitPipeline()
{
	pRenderer->DestroyGraphicsPipeline(imgui_pipeline);
}

void EmuUI::ResetIndirectDrawBuffer()
{
	for (int i = 0; i < indirect_draw_buffer_size; i++)
	{
		m_indexed_indirect_command[i].indexCount = 0;
		m_indexed_indirect_command[i].instanceCount = 0;
		m_indexed_indirect_command[i].firstIndex = 0;
		m_indexed_indirect_command[i].vertexOffset = 0;
		m_indexed_indirect_command[i].firstInstance = 0;
	}
}

void EmuUI::DockSpace()
{
	//ImGui::ShowDemoWindow();
	static ImGuiDockNodeFlags opt_flags = ImGuiDockNodeFlags_PassthruCentralNode;

	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
	// because it would be confusing to have two docking targets within each others.
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

	window_flags |= ImGuiWindowFlags_MenuBar;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::SetNextWindowBgAlpha(0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	// When using ImGuiDockNodeFlags_PassthruDockspace, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
	if (opt_flags & ImGuiDockNodeFlags_PassthruCentralNode)
		window_flags |= ImGuiWindowFlags_NoBackground;

	//ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(255, 255, 255, 255));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	static bool open = true;
	ImGui::Begin("Main DockSpace", &open, window_flags);
	ImGui::PopStyleVar();

	ImGui::PopStyleVar(2);


	ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(), ImVec2(1080,720), ImGui::ColorConvertFloat4ToU32(ImVec4(0.060f, 0.060f, 0.060f, 0.940f)));

		
	// Dockspace
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), opt_flags);
	}
	else
	{
		assert(0 && "Dockspace not enabled!!! Needed in experimental branch of imgui");
	}

	ImGui::End();
	//ImGui::PopStyleColor();
}

bool EmuUI::ElementClicked()
{
	return ImGui::IsItemHovered() && ImGui::IsMouseClicked( 0 );
}

void EmuUI::CalculateImageScaling( unsigned int image_width, unsigned int image_height, unsigned int window_width, unsigned int window_height, ImVec2& new_image_offset, ImVec2& new_image_size )
{
	// Used to offset the image to stop it going over the menu bar
	unsigned int menuBarHeight = GetMenuBarHeight();
	window_height -= menuBarHeight;
	float aspect = (float)image_height / (float)image_width;
	float childAspect = (float)window_height / (float)window_width;
	if (childAspect > aspect) // Fit to width
	{
		new_image_size.x = window_width;
		new_image_size.y = window_width * aspect;
		new_image_offset.x = 0;
		new_image_offset.y = (window_height - new_image_size.y) / 2 + menuBarHeight;
	}
	else // Fit to height
	{
		new_image_size.x = window_height / aspect;
		new_image_size.y = window_height;
		new_image_offset.x = (window_width - new_image_size.x) / 2;
		new_image_offset.y = menuBarHeight;
	}
}

void EmuUI::RenderMainMenuItem( std::string text, MenuItem* item )
{
	if (item->children.size() > 0)
	{
		if (ImGui::BeginMenu( text.c_str() ))
		{
			for (auto it = item->children.begin(); it != item->children.end(); ++it)
			{
				MarkSelectedElement( item->selected );
				RenderMainMenuItem( it->first, &it->second );
			}
			ImGui::EndMenu();
		}
	}
	else
	{
		if (ImGui::MenuItem( text.c_str() ))
		{
			MarkSelectedElement( item->selected );
		}
	}
}

void EmuUI::ImGuiCommandBufferCallback(VkCommandBuffer& command_buffer)
{
	vkCmdBindPipeline(
		command_buffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		imgui_pipeline.pipeline
	);
	// Texture Uniforms
	vkCmdBindDescriptorSets(
		command_buffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		imgui_pipeline.pipeline_layout,
		0,
		1,
		&imgui_texture_descriptor_set,
		0,
		NULL
	);
	// Gpu Context
	vkCmdBindDescriptorSets(
		command_buffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		imgui_pipeline.pipeline_layout,
		1,
		1,
		&imgui_gpu_context_descriptor_set,
		0,
		NULL
	);
	for (int i = 0 ; i < imgui_draw_instances.size();i++)
	{
		auto& draw_instance = imgui_draw_instances[i];

		VkRect2D& scissor = draw_instance.scissor;
		vkCmdSetScissor(command_buffer, 0, 1, &scissor);


		{	// Vertex Buffer
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(
				command_buffer,
				0,
				1,
				&imgui_vertex_buffer.buffer,
				offsets
			);

		}

		{	// Index Buffer
			vkCmdBindIndexBuffer(
				command_buffer,
				imgui_index_buffer.buffer,
				0,
				VK_INDEX_TYPE_UINT32
			);
		}

		{	// Texture Index Buffer
			VkDeviceSize offsets[] = { i * sizeof(int) };
			vkCmdBindVertexBuffers(
				command_buffer,
				1,
				1,
				&imgui_texture_buffer.buffer,
				offsets
			);
		}

		/*vkCmdDrawIndexed(
			command_buffer,
			draw_instance.indexSize,
			1,
			0,
			draw_instance.vertexOffset,
			0
		);*/

		vkCmdDrawIndexedIndirect(
			command_buffer,
			indexed_indirect_buffer.buffer,
			i * sizeof(VkDrawIndexedIndirectCommand),
			1,
			sizeof(VkDrawIndexedIndirectCommand));
	}
}

void EmuUI::RegisterWindow(Window* window)
{
	m_windows.push_back(window);
}

bool EmuUI::IsSelectedElement( std::string name )
{
	auto it = m_selected_element.find( name );

	if (it == m_selected_element.end()) return false;

	return it->second;
}

void EmuUI::MarkSelectedElement( std::string name )
{
	m_selected_element[name] = true;
}

void EmuUI::ResetSelectedElements()
{
	m_selected_element.clear();
}

void EmuUI::AddMenuItem( std::vector<std::string> path, std::string selected )
{
	if (path.size() == 0)return;


	std::map<std::string, MenuItem>* menu = &m_menu_items;

	MenuItem* currentItem = nullptr;

	for (int i = 0; i < path.size(); i++)
	{
		std::map<std::string, MenuItem>::const_iterator& it = menu->find( path[i] );
		
		if (it == menu->end())
		{
			(*menu)[path[i]] = MenuItem();
		}
		currentItem = &(*menu)[path[i]];

		menu = &currentItem->children;
	}

	currentItem->selected = selected;

	//m_menu_items = menu;
}

EmuRender::STexture& EmuUI::GetVisualisationTexture()
{
	return visualisation_texture;
}

bool EmuUI::IsWindowFocused()
{
	return ImGui::IsWindowFocused( ImGuiFocusedFlags_AnyWindow );
}

void EmuUI::DrawScalingImage( unsigned int texture_id, unsigned int image_width, unsigned int image_height, unsigned int window_width, unsigned int window_height )
{
	// Create pass by refrance variables to calculate image size and offset
	ImVec2 imageOffset;
	ImVec2 imageSize;
	// Calculate image size/offset
	CalculateImageScaling( image_width, image_height, window_width, window_height, imageOffset, imageSize );
	// Offset image
	ImGui::SetCursorPos( imageOffset );
	// Draw Image
	ImGui::Image( (ImTextureID)texture_id, imageSize, ImVec2( 0, 0 ), ImVec2( 1, 1 ), ImColor( 255, 255, 255, 255 ), ImColor( 255, 255, 255, 128 ) );
}

unsigned int EmuUI::GetMenuBarHeight()
{
	return ImGui::GetFont()->FontSize + ImGui::GetStyle().FramePadding.y * 2;
}

EmuUI* EmuUI::GetInstance()
{
	return instance;
}

void EmuUI::InitImGui()
{

	// Init ImGUI
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(pWindow->GetWidth(), pWindow->GetHeight());
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable | ImGuiBackendFlags_HasMouseHoveredViewport;
}
