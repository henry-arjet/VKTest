#pragma once
//Adapted from Vulkan-Tutorial
//This is the file that handles all the vulkan stuff
//Henry Arjet, 2020

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>
#include <arjet/shader.h>
#include <arjet/mesh.h>

#include <chrono>
#include <thread>
#include <iostream>
#include <vector>
#include <fstream>
#include <optional>
#include <array>
#include <string>
#include <mutex>

#include <stb_image.h>

#define cstr const char*
#define assres assert(res == VK_SUCCESS)
#define ushort uint16_t
#define uint uint32_t
#define ulong uint64_t
#define scuint static_cast<uint32_t>
#define ARJET_SHADER_FLAG_NORMAL 1 // flag for telling my shaders this mesh uses normal maps

using std::vector;
using std::cout;
using std::endl;

class Mesh;
class Model;

struct GraphicsOptions {
	bool vsync = false;
};

class Renderer {

public:
	
	GraphicsOptions graphicsOptions;

	//Validation layers for debugging
	const vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

	#ifdef NDEBUG
	const bool enableValidationLayers = false;
	#else
	const bool enableValidationLayers = true;
	#endif

	int width; //size of window we're drawing
	int height;

	bool framebufferResized = false; //keeps track if the window has been resized, neccessitating a swapchain recreation
	
	const int MAX_FRAMES_IN_FLIGHT = 2; //how many frames can be worked on at one time

	SDL_Window* window; // primary window

	VkInstance inst;
	VkResult res; //used to simplify error checking using the assres pattern
	VkSurfaceKHR surface; //primary surface to which the renderer draws
	VkPhysicalDevice physicalDevice; //The physical GPU we're using
	VkDevice device; //Vulkan likes virtual devices, so we create one from the above GPU
	VkSwapchainKHR swapchain = {};
	VkExtent2D swapchainExtent; //size of swapchain
	VkFormat swapchainImageFormat;
	VkRenderPass renderPass; //Using a single renderpass for now
	VkDescriptorSetLayout descriptorSetLayout; //Single, universal DS layout. UBO, diffuse and normal
	VkPipelineLayout pipelineLayout; //Using single UBO, so single pipeline layout
	vector<VkPipeline> graphicsPipelines; //one pipeline per shader
	vector<VkImageView> swapchainViews; //one swapchain image per frame in flight
	vector<VkFramebuffer> swapchainFramebuffers;
	vector<VkImage> swapchainImages;
	VkCommandPool commandPool; //Using single command pool because IDK what I'm doing
	uint queueFamilyIndex; //The queue family we want to use. Assumes graphics/present are the same queue family
	vector<VkCommandBuffer> commandBuffers; //Master command buffers, one per frame, recreated each frame
	vector<VkSemaphore> imageAvailableSemaphores;//like most things, one per frame
	vector<VkSemaphore> renderFinishedSemaphores;
	vector<VkFence> inFlightFences;
	vector<VkFence> imagesInFlight;
	VkQueue graphicsQueue; //We also assume this is the present queue
	
	//Texture vectors, one per texture
	//Might want to make texture class
	vector<VkImage> textureImages;
	vector<VkDeviceMemory> textureImageMemory;
	vector <VkImageView> textureImageViews;

	VkSampler textureSampler; //Simple sampler for textures, normal maps, specular
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	uint imageIndex;

	//Models
	vector<Model*> models;

	vector<ShaderPath> shaderPaths; //sets of shader paths we're going to call
	vector<Shader> shaders; //the index of the shader in the vector 'shaders' should match the shader's index such that shaders[i].index = i
	
	std::mutex threadLock;//only one thread can access the main resources at a time

	//just to make sure we're all on the same page
	uint currentFrame = 0;
	Renderer() {}
	Renderer(vector<ShaderPath> paths, int width = 1600, int height = 900) {
		init(paths, width, height);
	}
	void init(vector<ShaderPath> paths, int width = 1600, int height = 900) {
		this->width = width;
		this->height = height;
		this->shaderPaths = paths;
		initWindow();
		initInstance();
		createSurface();
		findPhysicalDevice();
		createLogicalDevice();
		createSwapchain();
		createRenderPass();
		createDescriptorSetLayout();
		buildShaders();
		createPipelines();
		createCommandPool();
		createDepthResources();
		createFramebuffers();
		createTextureSampler();

		createSyncObjects();
		createMasterCmdBuffers();

	}
	void createMasterCmdBuffers(); //creates the reusable master command buffers. One per frame

	void buildShaders(); //Turns shaderPaths into Shader objects. Assigns index by the index in shaderPaths

	bool checkValidationLayerSupport(); //Checks to see if the specified validation layers are supported

	uint findMemoryType(uint typeFilter, VkMemoryPropertyFlags properties); //finds the memory type that supports the given properties

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, 
		VkBuffer& buffer, VkDeviceMemory& bufferMemory); //creates a buffer and buffer memory with the given properties

	VkFormat findDepthFormat();

	VkCommandBuffer beginSingleTimeCommands(); //creates and starts recording a command optimized for runing once

	void endSingleTimeCommands(VkCommandBuffer commandBuffer); //Ends the the given buffer

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size); //copies the contents of a buffer to another buffer

	void createImage(uint width, uint height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, 
		VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory); //creates an image.

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint width, uint height); //takes a single purpose buffer and transfers it to a GPU image

	void createTextureImage(int index, cstr path);//fills the textureImage vars for a given index using a cstring image path. Called by model on load

	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT); //turns an image into an imageview

	void createTextureImageView(int index); //fills textureImageViews[index] with createImageView() SRGB call

	void createTextureSampler(); //creates our basic sampler

	VkFormat findSupportedFormat(const vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features); //

	bool hasStencilComponent(VkFormat format) { //finds if format has sencil component
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	} 

	void createDepthResources(); //creates and fills out the depth image stuff

	//void createDescriptorPool();

	void createSyncObjects(); //creates fences and semaphores

	void initWindow(); //creates a SDL/Vulkan window

	void initInstance(); //creates a Vulkan instance for SDL using the class vars

	void createSurface();

	void findPhysicalDevice(); //finds the first physical device. Doesn't work for more than one ie integrated and discrete

	void createLogicalDevice();//turns physical device into logical device

	void createSwapchain();

	void createDescriptorSetLayout(); //only using one descriptor set that contains a UBO, a texture, and a normal map

	void createPipelines(); //one pipeline for each shader
	
	void createFramebuffers();

	void createCommandPool(); //creates the master pool for all the buffers

	void createRenderPass();//again only one render pass

	vector<VkCommandBuffer> createCommandBuffersModel(vector<Mesh>* meshes); //Creates the sub command buffers that each model stores.
	//I would have it as const Mesh*, except I need to call getIndicesSize which is a non-static function

	void drawFrame();

	void recreateCommandBuffer();//Recreates the master buffer

	void cleanupSwapChain();

	void recreateSwapchain();


	void cleanModels();

	void cleanup();

	~Renderer();
};