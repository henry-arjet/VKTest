#pragma once
//Adapted from Vulkan-Tutorial
//This is the file that handles all the vulkan stuff
//Henry Arjet, 2020

#define VK_USE_PLATFORM_WIN32_KHR

//depth is -1 - 1 in GL and 0 - 1 in VK 
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED

#define cstr const char*
#define assres assert(res == VK_SUCCESS)
#define ushort uint16_t
#define uint uint32_t
#define scuint static_cast<uint32_t>

#define STB_IMAGE_IMPLEMENTATION

#include <arjet/vertex.h>

#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#include <chrono>
#include <iostream>
#include <vector>
#include <fstream>
#include <optional>
#include <array>
#include <string>

using std::vector;
using std::cout;
using std::endl;

using glm::vec2;
using glm::vec3;
using glm::mat4;

struct UniformBufferObject {
	alignas(16) mat4 model;
	alignas(16) mat4 view;
	alignas(16) mat4 proj;
};


const vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif


class Renderer {
public:
	int width = 1280; //defines the default resolution
	int height = 720; //depreciated, now defaulted in constructor

	UINT currentFrame = 0;
	bool framebufferResized = false;

	const int MAX_FRAMES_IN_FLIGHT = 2;

	SDL_Window* window;

	VkInstance inst;
	VkResult res;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice = NULL;
	VkDevice device;
	VkSwapchainKHR swapchain = {};
	VkExtent2D swapchainExtent;
	VkFormat swapchainImageFormat;
	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	vector<VkImageView> swapchainViews;
	vector<VkFramebuffer> swapchainFramebuffers;
	vector<VkImage> swapchainImages;
	VkCommandPool commandPool;
	uint queueFamilyIndex; //assumes single graphics/present queue family
	vector <VkCommandBuffer> commandBuffers;
	vector<VkSemaphore> imageAvailableSemaphores;
	vector<VkSemaphore> renderFinishedSemaphores;
	vector<VkFence> inFlightFences;
	vector<VkFence> imagesInFlight;
	VkQueue graphicsQueue;
	vector<VkImage> textureImages;
	vector<VkDeviceMemory> textureImageMemory;
	vector<VkImageView> textureImageViews;
	VkSampler textureSampler;
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	UniformBufferObject ubo;
	vector<std::string> texturePaths;
	UINT imageIndex; //this shouldn't have to be a class var, but here we are.

	//Per mesh stuff
	VkBuffer vertexBuffer;
	VkBuffer indexBuffer; //TODO make better
	vector<VkBuffer> uniformBuffers; //TODO update like descriptor 
	vector<uint> indicesSize; //number of indices for each mesh

	vector<VkDescriptorPool> descriptorPools; //Set of descriptor pools. One pool per mesh. Props could push these to mesh local
	vector<vector<VkDescriptorSet>> descriptorSets; //2d vector of descriptor sets, first dimension is per mesh, second is per frame
	uint maxDescriptors = 20;//the maximum number of descriptor SETS the renderer can support. Set before init. Used to initialize the pool



	//ushort swappingFrame = UINT16_MAX; //used in swapDescriptorSets
	//ushort swapIndices[2] = { 0,0 }; Might need later


	Renderer(int width = 1600, int height = 900) {
		this->width = width;
		this->height = height;
	}


	bool checkValidationLayerSupport() {
		unsigned int layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (cstr layerName : validationLayers) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}
		return true;
	}

	static vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file");
		}
		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();
		return buffer;
	}

	VkShaderModule createShaderModule(const std::vector<char>& code) {
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
		VkShaderModule shaderModule;
		VkResult res = vkCreateShaderModule(device, &createInfo, NULL, &shaderModule);
		assert(res == VK_SUCCESS);
		return shaderModule;
	}

	void createDescriptorSetLayout() {
		std::array<VkDescriptorSetLayoutBinding, 2> bindings = {};

		//Sets up a simple, reusable descriptor set for a UBO and a single texture
		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		bindings[0].binding = 0;
		bindings[0].descriptorCount = 1;
		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		
		
		bindings[1].binding = 1;
		bindings[1].descriptorCount = 1;
		bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[1].pImmutableSamplers = NULL;
		
		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		res = vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &descriptorSetLayout);
		assres;
	}

	void createPipeline() {
		auto vertShaderCode = readFile("Shaders/vert.spv");
		auto fragShaderCode = readFile("Shaders/frag.spv");

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		auto bindingDescription = Vertex::getBindingDesription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();
		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<UINT>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapchainExtent.width;
		viewport.height = (float)swapchainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = swapchainExtent;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = 1;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		VkDynamicState dynamicStates[] = { //trying to put this in fucked me up
			VK_DYNAMIC_STATE_VIEWPORT
		};

		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 1;
		dynamicState.pDynamicStates = dynamicStates;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};//Layout is attached to pipeline here
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 0;

		res = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &pipelineLayout);
		assres;

		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = 1;
		depthStencil.depthWriteEnable = 1;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = 0;
		depthStencil.stencilTestEnable = VK_FALSE;



		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDepthStencilState = &depthStencil;
		//pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;

		res = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline);
		assert(res == VK_SUCCESS);



		vkDestroyShaderModule(device, fragShaderModule, NULL);
		vkDestroyShaderModule(device, vertShaderModule, NULL);
	}

	void createRenderPass() {
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = swapchainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = findDepthFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		res = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
		assert(res == VK_SUCCESS);


	}

	void createFramebuffers() {
		swapchainFramebuffers.resize(swapchainViews.size());
		for (UINT i = 0; i < swapchainViews.size(); i++) {
			std::array<VkImageView, 2> attachments = { swapchainViews[i], depthImageView };

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = scuint(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapchainExtent.width;
			framebufferInfo.height = swapchainExtent.height;
			framebufferInfo.layers = 1;

			res = vkCreateFramebuffer(device, &framebufferInfo, NULL, &swapchainFramebuffers[i]);
			assert(res == VK_SUCCESS);
		}

	}

	void createCommandPool() {

		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.pNext = NULL;
		cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
		cmdPoolInfo.flags = 0;

		res = vkCreateCommandPool(device, &cmdPoolInfo, NULL, &commandPool);
		assres;
	}

	UINT findMemoryType(UINT typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
		for (UINT i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}
		throw std::runtime_error("failed to find suitable memory type");
	}

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		res = vkCreateBuffer(device, &bufferInfo, NULL, &buffer);
		assres;

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
		res = vkAllocateMemory(device, &allocInfo, NULL, &bufferMemory);
		assres;

		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

	VkCommandBuffer beginSingleTimeCommands() {
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;

	}

	void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, NULL);
		vkQueueWaitIdle(graphicsQueue);
		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferCopy copyRegion = {};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		endSingleTimeCommands(commandBuffer);


	}

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

		res = vkCreateImage(device, &imageInfo, NULL, &image);
		assres;

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
		res = vkAllocateMemory(device, &allocInfo, NULL, &imageMemory);
		assres;
		vkBindImageMemory(device, image, imageMemory, 0);

	}

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			if (hasStencilComponent(format)) {
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destStage;
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else throw std::invalid_argument("Unsupported layout transition");


		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destStage,
			0,
			0, NULL,
			0, NULL,
			1, &barrier
		);

		endSingleTimeCommands(commandBuffer);
	}

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		endSingleTimeCommands(commandBuffer);
	}

	void createTextureImage(int index) {

		int texWidth, texHeight, texChannels;

		stbi_uc* pixels = stbi_load(texturePaths[index].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;
		assert(pixels);

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(device, stagingBufferMemory);
		cout << "Mapped the texture staging buffer!\n";

		stbi_image_free(pixels);


		createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImages[index], textureImageMemory[index]);

		//TODO - do this all in single cmdbuffer. Create and use setupCommandBuffer() and flushSetupCommands() helper functions 
		//Execute asynchronously
		transitionImageLayout(textureImages[index], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyBufferToImage(stagingBuffer, textureImages[index], static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
		transitionImageLayout(textureImages[index], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(device, stagingBuffer, NULL);
		vkFreeMemory(device, stagingBufferMemory, NULL);

	}

	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT) {
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		res = vkCreateImageView(device, &viewInfo, NULL, &imageView);
		assres;
		return imageView;
	}

	void createTextureImageView(int index) {
		textureImageViews[index] = createImageView(textureImages[index], VK_FORMAT_R8G8B8A8_SRGB);
	}

	void createTextureSampler() {
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 16;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		res = vkCreateSampler(device, &samplerInfo, NULL, &textureSampler);
		assres;

	}

	VkFormat findSupportedFormat(const vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
		for (VkFormat format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}

		throw std::runtime_error("Failed to find supported format");
	}

	VkFormat findDepthFormat() {
		return findSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	bool hasStencilComponent(VkFormat format) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	void createDepthResources() {
		VkFormat depthFormat = findDepthFormat();
		createImage(swapchainExtent.width, swapchainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
		depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
		transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	}

	void createDescriptorPool() {

		descriptorPools.resize(maxDescriptors);
		for (int i = 0; i < maxDescriptors; ++i) { //creates a descriptor pool per mesh, as required by maxDescriptors
			std::vector<VkDescriptorPoolSize>  poolSizes(2); //UBO + texture. I should really automate this
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[0].descriptorCount = static_cast<uint>(swapchainImages.size());
			poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[1].descriptorCount = static_cast<uint>(swapchainImages.size());

			VkDescriptorPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.poolSizeCount = static_cast<uint>(poolSizes.size());
			poolInfo.pPoolSizes = poolSizes.data();
			poolInfo.maxSets = static_cast<uint>(swapchainImages.size());
			poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

			res = vkCreateDescriptorPool(device, &poolInfo, NULL, &descriptorPools[i]);
			assres;
		}
	}

	
	//
	//void createDescriptorSetsIndividually(uint uboIndex, uint texIndex) { //used to edit descriptor sets while in flight
	//	//replaces the for loop with current frame
	//	VkDescriptorSetLayout layout = descriptorSetLayout; //creates copy
	//	VkDescriptorSetAllocateInfo allocInfo = {};
	//	allocInfo.descriptorPool = descriptorPool;
	//	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	//	allocInfo.descriptorSetCount = static_cast<uint>(1);
	//	allocInfo.pSetLayouts = &layout;

	//	res = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[currentFrame]);
	//	assres;

	//	VkDescriptorBufferInfo bufferInfo = {};
	//	bufferInfo.buffer = uniformBuffers[currentFrame];
	//	bufferInfo.offset = 0;
	//	bufferInfo.range = sizeof(UniformBufferObject);

	//	std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
	//	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	//	descriptorWrites[0].dstSet = descriptorSets[currentFrame];
	//	descriptorWrites[0].dstBinding = 0;
	//	descriptorWrites[0].dstArrayElement = 0;
	//	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	//	descriptorWrites[0].descriptorCount = 1;
	//	descriptorWrites[0].pBufferInfo = &bufferInfo;

	//	//texture
	//	VkDescriptorImageInfo imageInfo = {};

	//	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	//	imageInfo.imageView = textureImageViews[texIndex];
	//	imageInfo.sampler = textureSampler;

	//	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	//	descriptorWrites[1].dstSet = descriptorSets[currentFrame];
	//	descriptorWrites[1].dstBinding = 1;
	//	descriptorWrites[1].dstArrayElement = 0;
	//	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	//	descriptorWrites[1].descriptorCount = 1;
	//	descriptorWrites[1].pImageInfo = &imageInfo;

	//	vkUpdateDescriptorSets(device, static_cast<uint>(descriptorWrites.size()), descriptorWrites.data(), 0, NULL);
	//}


	//void reCreateDescriptorSets() { //called from drawFrame
	//	vkFreeDescriptorSets(device, descriptorPool, 1, &descriptorSets[currentFrame]); //can only free one at a time because we need to not mess up the frame in flight
	//	createDescriptorSetsIndividually(swapIndices[0], swapIndices[1]);
	//}

	//void swapDescriptorSets(uint uboIndex, uint texIndex) {
	//	swapIndices[0] = uboIndex;
	//	swapIndices[1] = texIndex;
	//	swappingFrame = (currentFrame+MAX_FRAMES_IN_FLIGHT - 1)%MAX_FRAMES_IN_FLIGHT; //add MFIF to ensure we don't wrap. SwappinFrame is last frame
	//}

	void createCommandBuffers() {
		commandBuffers.resize(swapchainFramebuffers.size());
		assert(commandBuffers.size() > 0);

		VkCommandBufferAllocateInfo cmdInfo = {};
		cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdInfo.pNext = NULL;
		cmdInfo.commandPool = commandPool;
		cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdInfo.commandBufferCount = (UINT)commandBuffers.size();
		res = vkAllocateCommandBuffers(device, &cmdInfo, commandBuffers.data());

		assres;

		for (UINT i = 0; i < commandBuffers.size(); i++) {

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			res = vkBeginCommandBuffer(commandBuffers[i], &beginInfo);
			assres;

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = swapchainFramebuffers[i];
			renderPassInfo.renderArea.offset = { 0,0 };
			renderPassInfo.renderArea.extent = swapchainExtent;

			std::array<VkClearValue, 2> clearValues = {  };
			clearValues[0].color = { 0.3f, 0.3f, 0.3f, 1.0f };
			clearValues[1].depthStencil = { 1.0f, 0 };
			renderPassInfo.clearValueCount = scuint(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();
			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
			

			cout << descriptorSets.size()  << endl;
			for (int j = 0; j < descriptorSets.size(); j++) {
				cout << "TEST" << endl;

				VkDeviceSize offsets[] = { 0 };
				VkBuffer vertexBufferArray[] = { vertexBuffer};
				vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBufferArray, offsets);
				vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);//TODO need to make call to mesh's index buffer
				vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[j][i], 0, NULL);
				vkCmdDrawIndexed(commandBuffers[i], indicesSize[j], 1, 0, 0, 0);
				//throw;
			}
			vkCmdEndRenderPass(commandBuffers[i]);

			res = vkEndCommandBuffer(commandBuffers[i]);
			assres;


		}
	}

	void createSyncObjects() {
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		imagesInFlight.resize(swapchainImages.size(), NULL);

		VkSemaphoreCreateInfo semInfo = {};
		semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (UINT i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			res = vkCreateSemaphore(device, &semInfo, NULL, &imageAvailableSemaphores[i]);
			assres;
			res = vkCreateSemaphore(device, &semInfo, NULL, &renderFinishedSemaphores[i]);
			assres;
			res = vkCreateFence(device, &fenceInfo, NULL, &inFlightFences[i]);
			assres;
		}

	}

	void initWindow() {
		// Copied from template
		// Create an SDL window that supports Vulkan rendering.
		if (SDL_Init(SDL_INIT_VIDEO) != 0) {
			throw std::runtime_error("Could not initialize SDL.");
		}
		window = SDL_CreateWindow("Vulkan Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
		if (window == NULL) {
			throw std::runtime_error("Could not create SDL window.");
		}


	}

	void initInstance() {
		// Get WSI extensions from SDL (we can add more if we like - we just can't remove these)
		UINT extension_count;
		if (!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, NULL)) {
			std::cout << "Could not get the number of required instance extensions from SDL." << std::endl;
			throw std::runtime_error("Could not get the number of required instance extensions from SDL.");
		}
		std::vector<const char*> extensions(extension_count);
		if (!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, extensions.data())) {
			std::cout << "Could not get the names of required instance extensions from SDL." << std::endl;
			throw std::runtime_error("Could not get the names of required instance extensions from SDL");
		}


		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pNext = NULL;
		appInfo.pApplicationName = "Samples Tests";
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo instInfo = {};
		instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instInfo.pNext = NULL;
		instInfo.flags = 0;
		instInfo.pApplicationInfo = &appInfo;
		instInfo.enabledExtensionCount = extension_count;
		instInfo.ppEnabledExtensionNames = extensions.data();
		if (enableValidationLayers) {
			instInfo.enabledLayerCount = static_cast<unsigned int>(validationLayers.size());
			instInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else instInfo.enabledLayerCount = 0;





		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested but not available");
		}

		res = vkCreateInstance(&instInfo, NULL, &inst);
		if (res == VK_ERROR_INCOMPATIBLE_DRIVER) {
			std::cout << "Cannot find a compatible Vulkan ICD\n";
			exit(-1);
		}
		else if (res) {
			std::cout << "Unknown error\n";
			exit(-1);
		}
	}

	void createSurface() {
		if (!SDL_Vulkan_CreateSurface(window, static_cast<VkInstance>(inst), &surface)) {
			throw "Could not create a Vulkan surface.";
		}
	}

	void findPhysicalDevice() {
		unsigned int gpuCount;
		res = vkEnumeratePhysicalDevices(inst, &gpuCount, NULL);
		std::vector<VkPhysicalDevice> gpus(gpuCount);
		res = vkEnumeratePhysicalDevices(inst, &gpuCount, gpus.data());
		physicalDevice = gpus[0];//just picks the first GPU
	}

	void createLogicalDevice() {
		typedef unsigned int uint;
		uint queueFamilyCount;

		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
		std::vector<VkQueueFamilyProperties> queueProps(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueProps.data());

		VkDeviceQueueCreateInfo queueInfo = {};

		// Iterate over each queue to learn whether it supports presenting:
		VkBool32 *pSupportsPresent = (VkBool32 *)malloc(queueFamilyCount * sizeof(VkBool32));
		for (uint32_t i = 0; i < queueFamilyCount; i++) {
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &pSupportsPresent[i]);
		}

		// Search for a graphics and a present queue in the array of queue
		// families, try to find one that supports both
		uint gqFamilyIndex = UINT32_MAX;
		uint pqFamilyIndex = UINT32_MAX;
		for (uint32_t i = 0; i < queueFamilyCount; ++i) {
			if ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
				if (gqFamilyIndex == UINT32_MAX)
					gqFamilyIndex = i;

				if (pSupportsPresent[i] == VK_TRUE) {
					gqFamilyIndex = i;
					pqFamilyIndex = i;
					break;
				}
			}
		}

		if (pqFamilyIndex == UINT32_MAX) {
			// If didn't find a queue that supports both graphics and present, then
			// find a separate present queue.
			for (size_t i = 0; i < queueFamilyCount; ++i)
				if (pSupportsPresent[i] == VK_TRUE) {
					pqFamilyIndex = i;
					break;
				}
		}
		free(pSupportsPresent);
		assert(pqFamilyIndex == gqFamilyIndex); //lol nevermind
		queueInfo.queueFamilyIndex = gqFamilyIndex;
		queueFamilyIndex = gqFamilyIndex;

		float queuePriorities[1] = { 0.0 };
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.pNext = NULL;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = queuePriorities;

		std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		VkDeviceCreateInfo deviceInfo = {};
		deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceInfo.pNext = NULL;
		deviceInfo.queueCreateInfoCount = 1;
		deviceInfo.pQueueCreateInfos = &queueInfo;
		deviceInfo.enabledExtensionCount = 1;
		deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
		deviceInfo.enabledLayerCount = 0;
		deviceInfo.ppEnabledLayerNames = NULL;
		deviceInfo.pEnabledFeatures = &deviceFeatures;

		res = vkCreateDevice(physicalDevice, &deviceInfo, NULL, &device);
		assres;
		vkGetDeviceQueue(device, gqFamilyIndex, 0, &graphicsQueue);
	}

	void createSwapchain() { //Also inludes image views
		VkSwapchainCreateInfoKHR swapchainInfo = {};
		swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainInfo.pNext = NULL;
		swapchainInfo.surface = surface;

		UINT formatCount;
		res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);
		assert(res == VK_SUCCESS);
		std::vector<VkSurfaceFormatKHR> surfFormats(formatCount);
		res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfFormats.data());
		assert(res == VK_SUCCESS);

		if (surfFormats[0].format == VK_FORMAT_UNDEFINED)
			surfFormats[0].format = VK_FORMAT_R8G8B8A8_UNORM;
		swapchainImageFormat = surfFormats[0].format;
		swapchainInfo.imageFormat = surfFormats[0].format;

		VkSurfaceCapabilitiesKHR surfaceCapa;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapa);
		if (surfaceCapa.currentExtent.width != UINT32_MAX) { swapchainExtent = surfaceCapa.currentExtent; }
		else {
			SDL_GL_GetDrawableSize(window, &width, &height);
			swapchainExtent = { static_cast<UINT>(width), static_cast<UINT>(height) };
		}

		swapchainInfo.minImageCount = surfaceCapa.minImageCount + 1;
		swapchainInfo.imageExtent = swapchainExtent;
		swapchainInfo.preTransform = surfaceCapa.currentTransform;
		swapchainInfo.imageArrayLayers = 1;
		swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		swapchainInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; //We don't need to mess with others
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //we assume present and graphics queue is the same
		swapchainInfo.queueFamilyIndexCount = 0;
		swapchainInfo.pQueueFamilyIndices = NULL;
		swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;


		res = vkCreateSwapchainKHR(device, &swapchainInfo, NULL, &swapchain);
		assert(res == VK_SUCCESS);

		UINT swapchainCount;
		vkGetSwapchainImagesKHR(device, swapchain, &swapchainCount, NULL);
		swapchainImages.resize(swapchainCount);
		res = vkGetSwapchainImagesKHR(device, swapchain, &swapchainCount, swapchainImages.data());
		assert(res == VK_SUCCESS);

		swapchainViews.resize(swapchainImages.size());
		for (size_t i = 0; i < swapchainImages.size(); i++) {
			swapchainViews[i] = createImageView(swapchainImages[i], swapchainImageFormat);
		}
	}

	void initVulkan() {
		initWindow();
		initInstance();
		createSurface();
		findPhysicalDevice();
		createLogicalDevice();
		createSwapchain();
		createRenderPass();
		createDescriptorSetLayout();
		createPipeline();
		createCommandPool();
		createDepthResources();
		createFramebuffers(); //textures are handled differently so I can have multiple of them
		int texturePathsSize = texturePaths.size();
		textureImages.resize(texturePathsSize);
		textureImageMemory.resize(texturePathsSize);
		textureImageViews.resize(texturePathsSize);
		for (int i = 0; i < texturePathsSize; i++) {
			createTextureImage(i);
			createTextureImageView(i);
		}
		createTextureSampler();
		//createVertexBuffer();
		//createIndexBuffer();
		//createUniformBuffers();
		createDescriptorPool();
	}
	void finalizeVulkan() {
		createCommandBuffers(); //I need to have the descriptor sets from the objects ready before I can process these functions
		createSyncObjects();
	}

	void cleanupSwapchain() {
		for (auto framebuffer : swapchainFramebuffers) { vkDestroyFramebuffer(device, framebuffer, NULL); }
		vkFreeCommandBuffers(device, commandPool, static_cast<UINT>(commandBuffers.size()), commandBuffers.data());
		vkDestroyPipeline(device, graphicsPipeline, NULL);
		vkDestroyPipelineLayout(device, pipelineLayout, NULL);
		vkDestroyRenderPass(device, renderPass, NULL);

		for (auto imageView : swapchainViews)
			vkDestroyImageView(device, imageView, NULL);
		vkDestroySwapchainKHR(device, swapchain, NULL);

		vkDestroyImageView(device, depthImageView, NULL);
		vkDestroyImage(device, depthImage, NULL);
		vkFreeMemory(device, depthImageMemory, NULL);

		/*for (uint i = 0; i < swapchainImages.size(); i++) {
			vkDestroyBuffer(device, uniformBuffers[i], NULL);
			vkFreeMemory(device, uniformBuffersMemory[i], NULL);
		}*/
		for (uint i = 0; i < descriptorPools.size(); i++) {
			vkDestroyDescriptorPool(device, descriptorPools[i], NULL);
		}
	}

	void recreateSwapchain() {
		width = 0, height = 0;
		SDL_GL_GetDrawableSize(window, &width, &height);
		while (width == 0 || height == 0) {
			SDL_GL_GetDrawableSize(window, &width, &height);
			SDL_WaitEvent(NULL);
		}
		vkDeviceWaitIdle(device);
		cleanupSwapchain();

		createSwapchain();
		createRenderPass();
		createPipeline();
		createDepthResources();
		createFramebuffers();
		//createUniformBuffers();
		createDescriptorPool();
		createCommandBuffers();
	}



	void startFrame() {//Split into two parts so I can update uniform buffers. Janky as fuck, but it should work
		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		res = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], NULL, &imageIndex);
		if (res == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapchain();
			return;
		}
		else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image");
		}


		if (imagesInFlight[imageIndex] != NULL) {
			vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
		}
		imagesInFlight[imageIndex] = inFlightFences[currentFrame];


	}
	void finishFrame(){
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences(device, 1, &inFlightFences[currentFrame]);
		res = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
		assres;

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		VkSwapchainKHR swapchains[] = { swapchain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = &imageIndex;

		res = vkQueuePresentKHR(graphicsQueue, &presentInfo);//remember we asserted that graphicsQueue = presentQueue
		if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || framebufferResized) {
			framebufferResized = false;
			recreateSwapchain();
			return;
		}
		else if (res != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image");
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}



	void cleanup() { //TODO Need to add per mesh cleanup

		vkWaitForFences(device, MAX_FRAMES_IN_FLIGHT, inFlightFences.data(), VK_TRUE, UINT64_MAX);
		cleanupSwapchain();
		vkDestroySampler(device, textureSampler, NULL);
		for (int i = 0; i < textureImages.size(); i++) {
			vkDestroyImageView(device, textureImageViews[i], NULL);
			vkDestroyImage(device, textureImages[i], NULL);
			vkFreeMemory(device, textureImageMemory[i], NULL);
		}
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NULL);
		vkDestroyBuffer(device, vertexBuffer, NULL);
		//vkFreeMemory(device, vertexBufferMemory, NULL);
		vkDestroyBuffer(device, indexBuffer, NULL);
		//vkFreeMemory(device, indexBufferMemory, NULL);
		for (UINT i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(device, renderFinishedSemaphores[i], NULL);
			vkDestroySemaphore(device, imageAvailableSemaphores[i], NULL);
			vkDestroyFence(device, inFlightFences[i], NULL);
		}
		vkDestroyCommandPool(device, commandPool, NULL);
		vkDestroyDevice(device, NULL);
		vkDestroySurfaceKHR(inst, surface, NULL);
		vkDestroyInstance(inst, NULL);
	}
};
