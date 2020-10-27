#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <VkInitializers.hpp>
#include <VkCore.hpp>

#include <functional>

class EmuWindow;

class EmuRender
{
public:
	struct SGraphicsPipeline
	{
		VkPipeline pipeline;
		VkPipelineLayout pipeline_layout;
		uint32_t shader_count;
		const char** shader_paths;
		VkShaderStageFlagBits* shader_stages_bits;
		std::unique_ptr<VkShaderModule> shader_modules;
		uint32_t descriptor_set_layout_count;
		std::unique_ptr<VkDescriptorSetLayout> descriptor_set_layout;
		uint32_t vertex_input_attribute_description_count;
		std::unique_ptr<VkVertexInputAttributeDescription> vertex_input_attribute_descriptions;
		uint32_t vertex_input_binding_description_count;
		std::unique_ptr<VkVertexInputBindingDescription> vertex_input_binding_descriptions;
		uint32_t dynamic_state_count;
		VkDynamicState* dynamic_states;
	};

	struct SBuffer
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory buffer_memory = VK_NULL_HANDLE;
		unsigned int buffer_size;
		VkBufferUsageFlags usage;
		VkSharingMode sharing_mode;
		VkMemoryPropertyFlags buffer_memory_properties;
		// Raw pointer that will point to GPU memory
		void* mapped_buffer_memory = nullptr;
	};

	struct STexture
	{
		unsigned int width;
		unsigned int height;
		VkFormat format;

		VkImage image;
		VkDeviceMemory memory;
		VkImageView view;
		VkImageLayout layout;
		VkSampler sampler;
	};

	EmuRender(EmuWindow* window);
	~EmuRender();
	void Render();

	VkDevice GetDevice();

	void CreateGraphicsPipeline(SGraphicsPipeline& graphics_pipeline);

	void DestroyGraphicsPipeline(SGraphicsPipeline& graphics_pipeline);

	void CreateBuffer(SBuffer& buffer);

	void DestroyBuffer(SBuffer& buffer);

	void MapMemory(SBuffer& buffer);

	void UnMapMemory(SBuffer& buffer);

	void TransferToGPUBuffer(SBuffer& buffer, void* src, unsigned int size);

	void TransferToGPUTexture(SBuffer& src, STexture& dest);

	void RegisterCommandBufferCallback(std::function<void(VkCommandBuffer&)> callback);

	void RequestCommandBufferRebuild();

	void RequestRebuildRenderResources();

	void CreateTexture(STexture& texture);

	void CreateSampler(STexture& texture);

	void CreateDescriptorPool(VkDescriptorPool& descriptor_pool, VkDescriptorPoolSize* descriptor_sizes, unsigned int descriptor_size_count, unsigned int max_sets);

	void CreateDescriptorSetLayout(VkDescriptorSetLayout& descriptor_set_layout, VkDescriptorSetLayoutBinding* layout_bindings, unsigned int set_layout_binding_count);

	void AllocateDescriptorSet(VkDescriptorSet& descriptor_set, const VkDescriptorPool& descriptor_pool, const VkDescriptorSetLayout& descriptor_set_layout, uint32_t count);

	static EmuRender* GetInstance();
private:
	void StartRenderer();
	void InitVulkan();
	void DestroyVulkan();
	void CreateSurface();
	void CreateRenderResources();
	void DestroyRenderResources();
	void RebuildRenderResources();

	void BuildCommandBuffers(std::unique_ptr<VkCommandBuffer>& command_buffers, const uint32_t buffer_count);

	friend class Visualisation;

	static EmuRender* instance;

	EmuWindow* m_window;

	VkSurfaceCapabilitiesKHR surface_capabilities;
	VkSurfaceKHR surface;

	VkInstance vulkan_instance;
	VkDebugReportCallbackEXT debugger;
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;


	uint32_t physical_devices_queue_family = 0;
	VkPhysicalDeviceProperties physical_device_properties;
	VkPhysicalDeviceFeatures physical_device_features;
	VkPhysicalDeviceMemoryProperties physical_device_mem_properties;

	VkDevice device = VK_NULL_HANDLE;
	VkQueue graphics_queue = VK_NULL_HANDLE;
	VkQueue present_queue = VK_NULL_HANDLE;
	VkCommandPool command_pool;

	VkSurfaceFormatKHR surface_format;
	VkPresentModeKHR present_mode;

	VkSwapchainKHR swap_chain;
	std::unique_ptr<VkImage> swapchain_images;
	uint32_t swapchain_image_count;
	std::unique_ptr<VkImageView> swapchain_image_views;

	VkRenderPass renderpass = VK_NULL_HANDLE;
	std::unique_ptr<VkFramebuffer> framebuffers = nullptr;
	std::unique_ptr<VkHelper::VulkanAttachments> framebuffer_attachments = nullptr;

	uint32_t current_frame_index = 0; // What frame are we currently using
	std::unique_ptr<VkFence> fences = nullptr;

	VkSemaphore image_available_semaphore;
	VkSemaphore render_finished_semaphore;
	VkSubmitInfo sumbit_info = {};
	VkPresentInfoKHR present_info = {};
	VkPipelineStageFlags wait_stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	std::unique_ptr<VkCommandBuffer> graphics_command_buffers = nullptr;

	std::vector<std::function<void(VkCommandBuffer&)>> command_buffer_callbacks;

	bool request_rebuild_render_resources;
	bool request_rebuild_commandbuffers;
};