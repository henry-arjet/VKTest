#if defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XLIB_KHR
#elif defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#endif

// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED

#define cstr const char*
#define assres assert(res == VK_SUCCESS)
#define ushort uint16_t
#define uint uint32_t

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

using std::vector;
using glm::vec2;
using glm::vec3;
using glm::mat4;

struct Vertex {
	vec2 pos;
	vec3 color;
	
	static VkVertexInputBindingDescription getBindingDesription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}

};

struct UniformBufferObject {
	alignas(16) mat4 model;
	alignas(16) mat4 view;
	alignas(16) mat4 proj;
};

const vector<Vertex> vertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};
const vector<ushort> indices = { 0,1,2,2,3,0 };

const vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

int width = 1280;
int height = 720;

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
UINT queueFamilyIndex; //assumes single graphics/present queue family
VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;
vector<VkBuffer> uniformBuffers;
vector<VkDeviceMemory> uniformBuffersMemory;
VkDescriptorPool descriptorPool;
vector<VkDescriptorSet> descriptorSets;
vector <VkCommandBuffer> commandBuffers;
vector<VkSemaphore> imageAvailableSemaphores;
vector<VkSemaphore> renderFinishedSemaphores;
vector<VkFence> inFlightFences;
vector<VkFence> imagesInFlight;
VkQueue graphicsQueue;


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
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;
	
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
	
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	res = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &pipelineLayout);
	assres;
	
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

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
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
		VkImageView attachments[] = { swapchainViews[i] };
		
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
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

void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBufferAllocateInfo allocInfo = {};

	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer tempCmd;
	vkAllocateCommandBuffers(device, &allocInfo, &tempCmd);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(tempCmd, &beginInfo);

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(tempCmd, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(tempCmd);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &tempCmd;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, NULL);
	vkQueueWaitIdle(graphicsQueue); // should replace with fence to submit multiple buffer copies at a time
	vkFreeCommandBuffers(device, commandPool, 1, &tempCmd);


}

void createVertexBuffer() {
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
	
	
	
	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
	copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(device, stagingBuffer, NULL);
	vkFreeMemory(device, stagingBufferMemory, NULL);
}

void createIndexBuffer() {
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
	copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(device, stagingBuffer, NULL);
	vkFreeMemory(device, stagingBufferMemory, NULL);
}

void createUniformBuffers() {
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	size_t s = swapchainImages.size(); //might save a few cycles by not having to look up sci.size() thrice. Might not. IDK.
	uniformBuffers.resize(s);
	uniformBuffersMemory.resize(s);
	for (size_t i = 0; i < s; i++) {
		createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT 
			| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
	}
}

void createDescriptorPool() {
	VkDescriptorPoolSize poolSize = {};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint>(swapchainImages.size());

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = static_cast<uint>(swapchainImages.size());
	res = vkCreateDescriptorPool(device, &poolInfo, NULL, &descriptorPool);
	assres;
}

void createDescriptorSets() {
	size_t s = swapchainImages.size(); //again just tryna save a few cycles
	vector<VkDescriptorSetLayout> layouts(s, descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorSetCount = static_cast<uint>(s);
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize(s);
	res = vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data());
	assres;

	for (uint i = 0; i < s; i++) {
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);


		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;

		vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, NULL);

	}
}

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

		VkClearValue clearColor = { 0.3f, 0.3f, 0.3f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;
		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, NULL);
		vkCmdDrawIndexed(commandBuffers[i], static_cast<UINT>(indices.size()), 1, 0, 0, 0);
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
	window = SDL_CreateWindow("Vulkan Window", SDL_WINDOWPOS_CENTERED,	SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	if (window == NULL) {
		throw std::runtime_error( "Could not create SDL window.");
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


	float queuePriorities[1] = { 0.0 };
	queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfo.pNext = NULL;
	queueInfo.queueCount = 1;
	queueInfo.pQueuePriorities = queuePriorities;

	std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	VkDeviceCreateInfo deviceInfo = {};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pNext = NULL;
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.pQueueCreateInfos = &queueInfo;
	deviceInfo.enabledExtensionCount = 1;
	deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
	deviceInfo.enabledLayerCount = 0;
	deviceInfo.ppEnabledLayerNames = NULL;
	deviceInfo.pEnabledFeatures = NULL;

	res = vkCreateDevice(physicalDevice, &deviceInfo, NULL, &device);
	assres;
	vkGetDeviceQueue(device, gqFamilyIndex, 0, &graphicsQueue);
}

void createSwapchain(){ //Also inludes image views
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
	if (surfaceCapa.currentExtent.width != UINT32_MAX) { swapchainExtent = surfaceCapa.currentExtent;}
	else {
		SDL_GL_GetDrawableSize(window, &width, &height);
		swapchainExtent = {static_cast<UINT>(width), static_cast<UINT>(height)};
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
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapchainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapchainInfo.imageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		createInfo.flags = 0;
		createInfo.pNext = NULL;
		res = vkCreateImageView(device, &createInfo, NULL, &swapchainViews[i]);
		assert(res == VK_SUCCESS);
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
	createFramebuffers();
	createCommandPool();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
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

	for (uint i = 0; i < swapchainImages.size(); i++) {
		vkDestroyBuffer(device, uniformBuffers[i], NULL);
		vkFreeMemory(device, uniformBuffersMemory[i], NULL);
	}

	vkDestroyDescriptorPool(device, descriptorPool, NULL);
}

void recreateSwapchain() {
	width = 0, height = 0;
	SDL_GL_GetDrawableSize(window, &width, &height);
	while (width == 0 || height == 0){
		SDL_GL_GetDrawableSize(window, &width, &height);
		SDL_WaitEvent(NULL);
	}
	vkDeviceWaitIdle(device);
	cleanupSwapchain();

	createSwapchain();
	createRenderPass();
	createPipeline();
	createFramebuffers();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
}

void updateUniformBuffer(uint currentImage) {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo = {};
	ubo.model = glm::rotate(mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), swapchainExtent.width / (float)swapchainExtent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	void* data;
	vkMapMemory(device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device, uniformBuffersMemory[currentImage]);
}

void drawFrame() {
	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	UINT imageIndex;
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

	updateUniformBuffer(imageIndex);

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

void mainLoop() {
	bool stillRunning = true;
	while (stillRunning) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {

			case SDL_QUIT:
				stillRunning = false;
				break;
			case SDL_WINDOWEVENT:
				switch (event.window.event) {
				case SDL_WINDOWEVENT_RESIZED:
					framebufferResized = true;
					break;
				default:
					break;
				}

			default:
				// Do nothing.
				break;
			}
		}

		drawFrame();

	}
}

void cleanup() {

	cleanupSwapchain();
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NULL);
	vkDestroyBuffer(device, vertexBuffer, NULL);
	vkFreeMemory(device, vertexBufferMemory, NULL);
	vkDestroyBuffer(device, indexBuffer, NULL);
	vkFreeMemory(device, indexBufferMemory, NULL);
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

int main() {
	initVulkan();
	mainLoop();
	cleanup();
}