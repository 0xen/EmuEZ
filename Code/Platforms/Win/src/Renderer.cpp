#include <Renderer.hpp>
#include <Window.hpp>

#include <stdexcept>
#include <assert.h>

EmuRender::EmuRender(EmuWindow* window) : m_window(window)
{
	bool request_rebuild_render_resources = false;
	StartRenderer();
}

EmuRender::~EmuRender()
{
	DestroyVulkan();
}

void EmuRender::Render()
{// Find next image
	VkResult wait_for_fences = vkWaitForFences(
		device,
		1,
		&fences.get()[current_frame_index],
		VK_TRUE,
		UINT32_MAX
	);
	assert(wait_for_fences == VK_SUCCESS);

	VkResult acquire_next_image_result = vkAcquireNextImageKHR(
		device,
		swap_chain,
		UINT64_MAX,
		image_available_semaphore,
		VK_NULL_HANDLE,
		&current_frame_index
	);
	assert(acquire_next_image_result == VK_SUCCESS);

	// Reset and wait
	VkResult queue_idle_result = vkQueueWaitIdle(
		graphics_queue
	);
	assert(queue_idle_result == VK_SUCCESS);

	VkResult reset_fences_result = vkResetFences(
		device,
		1,
		&fences.get()[current_frame_index]
	);
	assert(reset_fences_result == VK_SUCCESS);

	sumbit_info.pCommandBuffers = &graphics_command_buffers.get()[current_frame_index];

	VkResult queue_submit_result = vkQueueSubmit(
		graphics_queue,
		1,
		&sumbit_info,
		fences.get()[current_frame_index]
	);
	assert(queue_submit_result == VK_SUCCESS);

	present_info.pImageIndices = &current_frame_index;

	VkResult queue_present_result = vkQueuePresentKHR(
		present_queue,
		&present_info
	);
	// If the window was resized or something else made the current render invalid, we need to rebuild all the
	// render resources
	if (queue_present_result == VK_ERROR_OUT_OF_DATE_KHR || request_rebuild_render_resources)
	{
		request_rebuild_render_resources = false;
		RebuildRenderResources();
	}

	assert(queue_present_result == VK_SUCCESS || queue_present_result == VK_ERROR_OUT_OF_DATE_KHR);


	VkResult device_idle_result = vkDeviceWaitIdle(device);
	assert(device_idle_result == VK_SUCCESS);
}

VkDevice EmuRender::GetDevice()
{
	return device;
}

void EmuRender::CreateGraphicsPipeline(SGraphicsPipeline& graphics_pipeline)
{
	graphics_pipeline.pipeline = VkHelper::CreateGraphicsPipeline(
		physical_device,
		device,
		renderpass,
		graphics_pipeline.pipeline_layout,
		graphics_pipeline.shader_count,
		graphics_pipeline.shader_paths,
		graphics_pipeline.shader_stages_bits,
		graphics_pipeline.shader_modules.get(),
		graphics_pipeline.descriptor_set_layout_count,
		graphics_pipeline.descriptor_set_layout.get(),
		graphics_pipeline.vertex_input_attribute_description_count,
		graphics_pipeline.vertex_input_attribute_descriptions.get(),
		graphics_pipeline.vertex_input_binding_description_count,
		graphics_pipeline.vertex_input_binding_descriptions.get(),
		graphics_pipeline.dynamic_state_count,
		graphics_pipeline.dynamic_states
	);
}

void EmuRender::DestroyGraphicsPipeline(SGraphicsPipeline& graphics_pipeline)
{
	vkDestroyPipeline(
		device,
		graphics_pipeline.pipeline,
		nullptr
	);

	vkDestroyPipelineLayout(
		device,
		graphics_pipeline.pipeline_layout,
		nullptr
	);

	for (uint32_t i = 0; i < graphics_pipeline.shader_count; i++)
	{
		vkDestroyShaderModule(
			device,
			graphics_pipeline.shader_modules.get()[i],
			nullptr
		);
	}
}

void EmuRender::CreateBuffer(SBuffer& buffer)
{
	VkHelper::CreateBuffer(
		device,                                            // What device are we going to use to create the buffer
		physical_device_mem_properties,                    // What memory properties are avaliable on the device
		buffer.buffer,                                     // What buffer are we going to be creating
		buffer.buffer_memory,                              // The output for the buffer memory
		buffer.buffer_size,                                // How much memory we wish to allocate on the GPU
		buffer.usage,                                      // What type of buffer do we want. Buffers can have multiple types, for example, uniform & vertex buffer.
														   // for now we want to keep the buffer spetilised to one type as this will allow vulkan to optimize the data.
		buffer.sharing_mode,                               // There are two modes, exclusive and concurrent. Defines if it can concurrently be used by multiple queue
														   // families at the same time
		buffer.buffer_memory_properties                    // What properties do we rquire of our memory
	);

}

void EmuRender::DestroyBuffer(SBuffer& buffer)
{
	// Now we unmap the data
	vkUnmapMemory(
		device,
		buffer.buffer_memory
	);

	// Clean up the buffer data
	vkDestroyBuffer(
		device,
		buffer.buffer,
		nullptr
	);

	// Free the memory that was allocated for the buffer
	vkFreeMemory(
		device,
		buffer.buffer_memory,
		nullptr
	);

}

void EmuRender::MapMemory(SBuffer& buffer)
{
	// Get the pointer to the GPU memory
	VkResult mapped_memory_result = vkMapMemory(
		device,                                                         // The device that the memory is on
		buffer.buffer_memory,                                           // The device memory instance
		0,                                                              // Offset from the memorys start that we are accessing
		buffer.buffer_size,                                             // How much memory are we accessing
		0,                                                              // Flags (we dont need this for basic buffers)
		&buffer.mapped_buffer_memory                                    // The return for the memory pointer
	);
	// Could we map the GPU memory to our CPU accessable pointer
	assert(mapped_memory_result == VK_SUCCESS);
}

void EmuRender::UnMapMemory(SBuffer& buffer)
{
	vkUnmapMemory(
		device,                                                         // The device that the memory is on
		buffer.buffer_memory                                            // The device memory instance
	);
}

void EmuRender::TransferToGPUBuffer(SBuffer& buffer, void* src, unsigned int size)
{
	memcpy(
		buffer.mapped_buffer_memory,                         // The destination for our memory (GPU)
		src,                                                 // Source for the memory (CPU-Ram)
		size                                                 // How much data we are transfering
	);
}

void EmuRender::TransferToGPUTexture(SBuffer& src, STexture& dest)
{
	VkCommandBuffer copy_cmd = VkHelper::BeginSingleTimeCommands(device, command_pool);

	// The sub resource range describes the regions of the image we will be transition
	VkImageSubresourceRange subresourceRange = {};
	// Image only contains color data
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	// Start at first mip level
	subresourceRange.baseMipLevel = 0;
	// We will transition on all mip levels
	subresourceRange.levelCount = 1;
	// The 2D texture only has one layer
	subresourceRange.layerCount = 1;

	// Optimal image will be used as destination for the copy, so we must transfer from our
	// initial undefined image layout to the transfer destination layout

	VkHelper::SetImageLayout(
		copy_cmd,
		dest.image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		subresourceRange//,
		//VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		//VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
	);

	// Only dealing with one mip level for now
	VkBufferImageCopy bufferCopyRegion = {};
	bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	bufferCopyRegion.imageSubresource.mipLevel = 0;
	bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
	bufferCopyRegion.imageSubresource.layerCount = 1;
	bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(dest.width);
	bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(dest.height);
	bufferCopyRegion.imageExtent.depth = 1;
	bufferCopyRegion.bufferOffset = 0;

	// Copy mip levels from staging buffer
	vkCmdCopyBufferToImage(
		copy_cmd,
		src.buffer,
		dest.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&bufferCopyRegion);


	VkHelper::SetImageLayout(
		copy_cmd,
		dest.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		dest.layout,
		subresourceRange//,
		//VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		//VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
	);

	VkHelper::EndSingleTimeCommands(
		device,
		graphics_queue,
		copy_cmd,
		command_pool
		);

	vkFreeCommandBuffers(
		device,
		command_pool,
		1,
		&copy_cmd
	);

}

void EmuRender::RegisterCommandBufferCallback(std::function<void(VkCommandBuffer&)> callback)
{
	command_buffer_callbacks.push_back(callback);
	request_rebuild_render_resources = true;
}

void EmuRender::CreateTexture(STexture& texture)
{
	VkHelper::CreateImage(
		device,
		physical_device_mem_properties,
		texture.width,
		texture.height,
		texture.format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		texture.image,
		texture.memory,
		VK_IMAGE_LAYOUT_UNDEFINED
	);
}

void EmuRender::CreateSampler(STexture& texture)
{

	VkHelper::CreateImageSampler(
		device,
		texture.image,
		texture.format,
		texture.view,
		texture.sampler
	);
}

void EmuRender::StartRenderer()
{

	InitVulkan();

	BuildCommandBuffers(graphics_command_buffers, swapchain_image_count);
}

void EmuRender::InitVulkan()
{
	// Define what Layers and Extentions we require
	const uint32_t extention_count = 3;
	const char* instance_extensions[extention_count] = { "VK_EXT_debug_report" ,VK_KHR_SURFACE_EXTENSION_NAME,VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
	const uint32_t layer_count = 1;
	const char* instance_layers[layer_count] = { "VK_LAYER_LUNARG_standard_validation" };


	// Check to see if we have the layer requirments
	assert(VkHelper::CheckLayersSupport(instance_layers, 1) && "Unsupported Layers Found");

	// Create the Vulkan Instance
	instance = VkHelper::CreateInstance(
		instance_extensions, extention_count,
		instance_layers, layer_count,
		"EmuEZ", VK_MAKE_VERSION(1, 0, 0),
		"Vulkan", VK_MAKE_VERSION(1, 0, 0),
		VK_MAKE_VERSION(1, 1, 108));

	// Attach a debugger to the application to give us validation feedback.
	// This is usefull as it tells us about any issues without application
	debugger = VkHelper::CreateDebugger(instance);

	// Define what Device Extentions we require
	const uint32_t physical_device_extention_count = 1;
	// Note that this extention list is diffrent from the instance on as we are telling the system what device settings we need.
	const char* physical_device_extensions[physical_device_extention_count] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };



	// The surface creation is added here as it needs to happen before the physical device creation and after we have said to the instance that we need a surface
	// The surface is the refrence back to the OS window
	CreateSurface();



	// Find a physical device for us to use
	bool foundPhysicalDevice = VkHelper::GetPhysicalDevice(
		instance,
		physical_device,                                       // Return of physical device instance
		physical_device_properties,                            // Physical device properties
		physical_devices_queue_family,                         // Physical device queue family
		physical_device_features,                              // Physical device features
		physical_device_mem_properties,                        // Physical device memory properties
		physical_device_extensions,                            // What extentions out device needs to have
		physical_device_extention_count,                       // Extention count
		VK_QUEUE_GRAPHICS_BIT,                                 // What queues we need to be avaliable
		surface                                                // Pass the instance to the OS monitor
	);

	// Make sure we found a physical device
	assert(foundPhysicalDevice);

	// Define how many queues we will need in our project, for now, we will just create a single queue
	static const float queue_priority = 1.0f;
	VkDeviceQueueCreateInfo queue_create_info = VkHelper::DeviceQueueCreateInfo(
		&queue_priority,
		1,
		physical_devices_queue_family
	);


	// Now we have the physical device, create the device instance
	device = VkHelper::CreateDevice(
		physical_device,                                       // The physical device we are basic the device from
		&queue_create_info,                                    // A pointer array, pointing to a list of queues we want to make
		1,                                                     // How many queues are in the list
		physical_device_features,                              // What features do you want enabled on the device
		physical_device_extensions,                            // What extentions do you want on the device
		physical_device_extention_count                        // How many extentions are there
	);

	vkGetDeviceQueue(
		device,
		physical_devices_queue_family,
		0,
		&graphics_queue
	);

	vkGetDeviceQueue(
		device,
		physical_devices_queue_family,
		0,
		&present_queue
	);

	command_pool = VkHelper::CreateCommandPool(
		device,
		physical_devices_queue_family,                         // What queue family we are wanting to use to send commands to the GPU
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT        // Allows any commands we create, the ability to be reset. This is helpfull as we wont need to
	);                                                         // keep allocating new commands, we can reuse them


	CreateRenderResources();

	fences = std::unique_ptr<VkFence>(new VkFence[swapchain_image_count]);

	for (unsigned int i = 0; i < swapchain_image_count; i++)
	{
		VkFenceCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		VkResult create_fence_result = vkCreateFence(
			device,
			&info,
			nullptr,
			&fences.get()[i]
		);
		assert(create_fence_result == VK_SUCCESS);
	}


	VkSemaphoreCreateInfo semaphore_create_info = {};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkResult create_semaphore_result = vkCreateSemaphore(device,
		&semaphore_create_info,
		nullptr,
		&image_available_semaphore
	);
	assert(create_semaphore_result == VK_SUCCESS);

	create_semaphore_result = vkCreateSemaphore(device,
		&semaphore_create_info,
		nullptr,
		&render_finished_semaphore
	);
	assert(create_semaphore_result == VK_SUCCESS);


	VkCommandBufferAllocateInfo command_buffer_allocate_info = VkHelper::CommandBufferAllocateInfo(
		command_pool,
		swapchain_image_count
	);

	graphics_command_buffers = std::unique_ptr<VkCommandBuffer>(new VkCommandBuffer[swapchain_image_count]);

	VkResult allocate_command_buffer_resut = vkAllocateCommandBuffers(
		device,
		&command_buffer_allocate_info,
		graphics_command_buffers.get()
	);
	assert(allocate_command_buffer_resut == VK_SUCCESS);

	sumbit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	sumbit_info.waitSemaphoreCount = 1;
	sumbit_info.pWaitSemaphores = &image_available_semaphore;
	sumbit_info.pWaitDstStageMask = &wait_stages;
	sumbit_info.commandBufferCount = 1;
	sumbit_info.signalSemaphoreCount = 1;
	sumbit_info.pSignalSemaphores = &render_finished_semaphore;

	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &render_finished_semaphore;
	present_info.swapchainCount = 1;
	// The swapchain will be recreated whenevr the window is resized or the KHR becomes invalid
	// But the pointer to our swapchain will remain intact
	present_info.pSwapchains = &swap_chain;
	present_info.pResults = nullptr;
}

void EmuRender::DestroyVulkan()
{
	DestroyRenderResources();

	for (uint32_t i = 0; i < swapchain_image_count; i++)
	{
		vkDestroyFence(
			device,
			fences.get()[i],
			nullptr
		);
	}

	vkDestroySemaphore(
		device,
		image_available_semaphore,
		nullptr
	);

	vkDestroySemaphore(
		device,
		render_finished_semaphore,
		nullptr
	);


	// Clean up the command pool
	vkDestroyCommandPool(
		device,
		command_pool,
		nullptr
	);

	// Clean up the device now that the project is stopping
	vkDestroyDevice(
		device,
		nullptr
	);

	// Destroy the debug callback
	// We cant directly call vkDestroyDebugReportCallbackEXT as we need to find the pointer within the Vulkan DLL, See function inplmentation for details.
	VkHelper::DestroyDebugger(
		instance,
		debugger
	);

	// Clean up the vulkan instance
	vkDestroyInstance(
		instance,
		NULL
	);

}

void EmuRender::CreateSurface()
{
	auto CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");

	VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = m_window->GetWindowInfo().info.win.window;
	createInfo.hinstance = GetModuleHandle(nullptr);

	if (!CreateWin32SurfaceKHR || CreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface!");
	}
}

void EmuRender::CreateRenderResources()
{
	swap_chain = VkHelper::CreateSwapchain(
		physical_device,
		device,
		surface,
		surface_capabilities,
		surface_format,
		present_mode,
		m_window->GetWidth(),
		m_window->GetHeight(),
		swapchain_image_count,
		swapchain_images,
		swapchain_image_views
	);


	const VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
	renderpass = VkHelper::CreateRenderPass(
		physical_device,
		device,
		surface_format.format,
		colorFormat,
		swapchain_image_count,
		physical_device_mem_properties,
		physical_device_features,
		physical_device_properties,
		command_pool,
		graphics_queue,
		m_window->GetWidth(),
		m_window->GetHeight(),
		framebuffers,
		framebuffer_attachments,
		swapchain_image_views
	);
}

void EmuRender::DestroyRenderResources()
{
	vkDestroyRenderPass(
		device,
		renderpass,
		nullptr
	);

	vkDestroySwapchainKHR(
		device,
		swap_chain,
		nullptr
	);

	for (uint32_t i = 0; i < swapchain_image_count; i++)
	{
		vkDestroyImageView(
			device,
			swapchain_image_views.get()[i],
			nullptr
		);


		vkDestroyFramebuffer(
			device,
			framebuffers.get()[i],
			nullptr
		);

		vkDestroyImageView(
			device,
			framebuffer_attachments.get()[i].color.view,
			nullptr
		);
		vkDestroyImage(
			device,
			framebuffer_attachments.get()[i].color.image,
			nullptr
		);
		vkFreeMemory(
			device,
			framebuffer_attachments.get()[i].color.memory,
			nullptr
		);
		vkDestroySampler(
			device,
			framebuffer_attachments.get()[i].color.sampler,
			nullptr
		);
		vkDestroyImageView(
			device,
			framebuffer_attachments.get()[i].depth.view,
			nullptr
		);
		vkDestroyImage(
			device,
			framebuffer_attachments.get()[i].depth.image,
			nullptr
		);
		vkFreeMemory(
			device,
			framebuffer_attachments.get()[i].depth.memory,
			nullptr
		);
		vkDestroySampler(
			device,
			framebuffer_attachments.get()[i].depth.sampler,
			nullptr
		);
	}
}

void EmuRender::RebuildRenderResources()
{
	VkResult device_idle_result = vkDeviceWaitIdle(device);
	assert(device_idle_result == VK_SUCCESS);

	DestroyRenderResources();
	CreateRenderResources();
	BuildCommandBuffers(graphics_command_buffers, swapchain_image_count);
}

void EmuRender::BuildCommandBuffers(std::unique_ptr<VkCommandBuffer>& command_buffers, const uint32_t buffer_count)
{
	// Create the info to build a new render pass
	VkCommandBufferBeginInfo command_buffer_begin_info = VkHelper::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

	float* clear_color = m_window->GetClearColor();

	VkClearValue clear_values[3]{};

	memcpy(clear_values[0].color.float32, clear_color, sizeof(float) * 4);
	memcpy(clear_values[1].color.float32, clear_color, sizeof(float) * 4);


	clear_values[2].depthStencil = { 1.0f, 0 };                                                           // Depth Image


	VkRenderPassBeginInfo render_pass_info = VkHelper::RenderPassBeginInfo(
		renderpass,
		{ 0,0 },
		{
			m_window->GetWidth(),
			m_window->GetHeight()
		}
	);

	render_pass_info.clearValueCount = 3;
	render_pass_info.pClearValues = clear_values;

	VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	// Loop through for each swapchain image and prepare there image
	for (unsigned int i = 0; i < buffer_count; i++)
	{
		// Reset the command buffers
		VkResult reset_command_buffer_result = vkResetCommandBuffer(
			command_buffers.get()[i],
			0
		);
		assert(reset_command_buffer_result == VK_SUCCESS);

		render_pass_info.framebuffer = framebuffers.get()[i];

		VkResult begin_command_buffer_result = vkBeginCommandBuffer(
			command_buffers.get()[i],
			&command_buffer_begin_info
		);
		assert(begin_command_buffer_result == VK_SUCCESS);

		vkCmdBeginRenderPass(
			command_buffers.get()[i],
			&render_pass_info,
			VK_SUBPASS_CONTENTS_INLINE
		);

		// Define how large the laser will be
		vkCmdSetLineWidth(
			command_buffers.get()[i],
			1.0f
		);

		VkViewport viewport = VkHelper::Viewport(
			(float)m_window->GetWidth(),
			(float)m_window->GetHeight(),
			0,
			0,
			0.0f,
			1.0f
		);

		vkCmdSetViewport(
			command_buffers.get()[i],
			0,
			1,
			&viewport
		);

		VkRect2D scissor{};
		scissor.extent.width = m_window->GetWidth();
		scissor.extent.height = m_window->GetHeight();
		scissor.offset.x = 0;
		scissor.offset.y = 0;

		vkCmdSetScissor(
			command_buffers.get()[i],
			0,
			1,
			&scissor
		);


		for (auto& callback : command_buffer_callbacks)
		{
			callback(command_buffers.get()[i]);
		}



		vkCmdEndRenderPass(
			command_buffers.get()[i]
		);

		VkResult end_command_buffer_result = vkEndCommandBuffer(
			command_buffers.get()[i]
		);
		assert(end_command_buffer_result == VK_SUCCESS);

	}
}
