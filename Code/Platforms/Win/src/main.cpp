#include <iostream>

#include <Window.hpp>
#include <Renderer.hpp>


// ImGUI
#include <imgui.h>

// Output when the logic steps happen on threads
#define VERBOSE_LOGIC 1

struct ImGUIDrawInstance
{
	unsigned int indexOffset;
	unsigned int indexSize;
	unsigned int vertexOffset;
	VkRect2D scissor;
};

std::unique_ptr<EmuWindow> pWindow;
std::unique_ptr<EmuRender> pRenderer;

EmuRender::SGraphicsPipeline imgui_pipeline;

struct mat4
{
	float data[16];
};

EmuRender::SBuffer imgui_vertex_buffer;
ImDrawVert* imgui_vertex_data = nullptr;
EmuRender::SBuffer imgui_index_buffer;
uint32_t* imgui_index_data = nullptr;
EmuRender::SBuffer imgui_texture_buffer;
int* imgui_texture_data = nullptr;

EmuRender::SBuffer imgui_font_texture_buffer;
EmuRender::STexture imgui_font_texture;

VkDescriptorPool imgui_texture_descriptor_pool;
VkDescriptorSetLayout imgui_texture_descriptor_set_layout;
VkDescriptorSet imgui_texture_descriptor_set;


std::vector<ImGUIDrawInstance> imgui_draw_instances;

void Log(const char* text)
{
#if VERBOSE_LOGIC
	std::cout << text << std::endl;
#endif
}

void CreateImGuiPipeline()
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

	const uint32_t descriptor_set_layout_count = 1;

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
	imgui_pipeline.descriptor_set_layout = std::unique_ptr<VkDescriptorSetLayout>(new VkDescriptorSetLayout[vertex_input_attribute_description_count]);;

	{
		imgui_pipeline.descriptor_set_layout.get()[0] = imgui_texture_descriptor_set_layout;
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

void DestroyImGuiPipeline()
{
	pRenderer->DestroyGraphicsPipeline(imgui_pipeline);
}

void CreateImGUIBuffers()
{
	{
		const unsigned int vertex_buffer_size = 30000;

		imgui_vertex_data = new ImDrawVert[vertex_buffer_size];

		imgui_vertex_buffer.buffer_size = sizeof(ImDrawVert) * vertex_buffer_size;
		imgui_vertex_buffer.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		imgui_vertex_buffer.sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
		imgui_vertex_buffer.buffer_memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		pRenderer->CreateBuffer(imgui_vertex_buffer);
		pRenderer->MapMemory(imgui_vertex_buffer);
	}
	{
		const unsigned int index_buffer_size = 40000;

		imgui_index_data = new uint32_t[index_buffer_size];

		imgui_index_buffer.buffer_size = sizeof(uint32_t) * index_buffer_size;
		imgui_index_buffer.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		imgui_index_buffer.sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
		imgui_index_buffer.buffer_memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		pRenderer->CreateBuffer(imgui_index_buffer);
		pRenderer->MapMemory(imgui_index_buffer);
	}
	{
		const unsigned int texture_buffer_size = 100;

		imgui_texture_data = new int[texture_buffer_size];

		imgui_texture_buffer.buffer_size = sizeof(int) *texture_buffer_size;
		imgui_texture_buffer.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		imgui_texture_buffer.sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
		imgui_texture_buffer.buffer_memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		pRenderer->CreateBuffer(imgui_texture_buffer);
		pRenderer->MapMemory(imgui_texture_buffer);
	}
	{
		ImGuiIO& io = ImGui::GetIO();

		// Create font texture
		unsigned char* font_data;
		int font_width, font_height;
		io.Fonts->GetTexDataAsRGBA32(&font_data, &font_width, &font_height);
		//m_imgui.m_font_texture = m_renderer->CreateTextureBuffer(font_data, VkFormat::VK_FORMAT_R8G8B8A8_UNORM, font_width, font_height);




		const unsigned int texture_size = font_width * font_height * 4;




		imgui_font_texture_buffer.buffer_size = texture_size;
		imgui_font_texture_buffer.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		imgui_font_texture_buffer.sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
		imgui_font_texture_buffer.buffer_memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		pRenderer->CreateBuffer(imgui_font_texture_buffer);
		pRenderer->MapMemory(imgui_font_texture_buffer);
		pRenderer->TransferToGPUBuffer(imgui_font_texture_buffer, font_data, texture_size);


		imgui_font_texture.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imgui_font_texture.format = VK_FORMAT_R8G8B8A8_UNORM;
		imgui_font_texture.width = font_width;
		imgui_font_texture.height = font_height;

		pRenderer->CreateTexture(imgui_font_texture);
		pRenderer->TransferToGPUTexture(imgui_font_texture_buffer, imgui_font_texture);
		pRenderer->CreateSampler(imgui_font_texture);


		io.Fonts->TexID = (ImTextureID)1;
	}

}

void DestroyImGUIBuffers()
{
	pRenderer->DestroyBuffer(imgui_texture_buffer);
	pRenderer->DestroyBuffer(imgui_index_buffer);
	pRenderer->DestroyBuffer(imgui_vertex_buffer);
}

void CreateImGuiDescriptors()
{

	const uint32_t descriptor_pool_size_count = 1;

	VkDescriptorPoolSize pool_size[descriptor_pool_size_count] = {
		VkHelper::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
	};

	pRenderer->CreateDescriptorPool(imgui_texture_descriptor_pool, pool_size, descriptor_pool_size_count, 10);

	VkDescriptorSetLayoutBinding layout_bindings[descriptor_pool_size_count] = {
		VkHelper::DescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
	};

	pRenderer->CreateDescriptorSetLayout(imgui_texture_descriptor_set_layout, layout_bindings, descriptor_pool_size_count);

	pRenderer->AllocateDescriptorSet(imgui_texture_descriptor_set, imgui_texture_descriptor_pool, imgui_texture_descriptor_set_layout, 1);

	std::vector<VkDescriptorImageInfo> descriptorImageInfos;
	{
		VkDescriptorImageInfo info;
		info.sampler = imgui_font_texture.sampler;
		info.imageView = imgui_font_texture.view;
		info.imageLayout = imgui_font_texture.layout;
		descriptorImageInfos.push_back(info);
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

void ImGuiCommandBufferCallback(VkCommandBuffer& command_buffer)
{
	vkCmdBindPipeline(
		command_buffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		imgui_pipeline.pipeline
	);
	//
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
	for (auto& draw_instance : imgui_draw_instances)
	{
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
				draw_instance.indexOffset,
				VK_INDEX_TYPE_UINT32
			);
		}

		{	// Texture Index Buffer
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(
				command_buffer,
				1,
				1,
				&imgui_texture_buffer.buffer,
				offsets
			);
		}

		vkCmdDrawIndexed(
			command_buffer,
			draw_instance.indexSize,
			1,
			0,
			draw_instance.vertexOffset,
			0
		);
	}
}

void Setup()
{
	// Create window
	pWindow = std::make_unique<EmuWindow>("EmuEZ", 1080, 720);
	// Create renderer instance
	pRenderer = std::make_unique<EmuRender>(pWindow.get());
	// Render a initial blank frame
	pRenderer->Render();
	pWindow->OpenWindow();

	// Init ImGUI
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(pWindow->GetWidth(), pWindow->GetHeight());
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

	CreateImGUIBuffers();
	CreateImGuiDescriptors();
	CreateImGuiPipeline();

	pRenderer->RegisterCommandBufferCallback(ImGuiCommandBufferCallback);
}

void Close()
{
	DestroyImGUIBuffers();
	DestroyImGuiPipeline();

	pRenderer.reset();
	pWindow.reset();
}

void UpdateImGUI()
{
	ImGui::NewFrame();
	ImGui::ShowDemoWindow();
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

				imgui_draw_instances[drawGroup].indexOffset = pcmd->IdxOffset + index_count;
				imgui_draw_instances[drawGroup].vertexOffset = pcmd->VtxOffset + vertex_count;
				imgui_draw_instances[drawGroup].indexSize = pcmd->ElemCount;
				// To do, texture id

				// Define the texture for the imgui render call
				//m_imgui.draw_group[drawGroup]->SetData(0, (int)pcmd->TextureId);
				temp_texture_data[drawGroup] = (int)pcmd->TextureId;

				drawGroup++;
			}
		}

		temp_vertex_data += cmd_list->VtxBuffer.Size;
		temp_index_data += cmd_list->IdxBuffer.Size;

		vertex_count += cmd_list->VtxBuffer.Size;
		index_count += cmd_list->IdxBuffer.Size;
	}
	pRenderer->TransferToGPUBuffer(imgui_vertex_buffer, imgui_vertex_data, imgui_vertex_buffer.buffer_size);

	pRenderer->TransferToGPUBuffer(imgui_index_buffer, imgui_index_data, imgui_index_buffer.buffer_size);

	pRenderer->TransferToGPUBuffer(imgui_texture_buffer, imgui_texture_data, imgui_texture_buffer.buffer_size);
}

int main(int, char**)
{
	Setup();
	





	EmuWindow::EWindowStatus windowStatus;
	while((windowStatus = pWindow->GetStatus()) != EmuWindow::EWindowStatus::Exiting)
	{

		UpdateImGUI();





		pRenderer->Render();
		pWindow->Poll();
	}

	if (windowStatus == EmuWindow::EWindowStatus::Exiting || windowStatus == EmuWindow::EWindowStatus::Open)
	{
		Close();
	}


	system("pause");
	return 0;
}