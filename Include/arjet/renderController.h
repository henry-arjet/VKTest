#pragma once

//This object handles all the rendering related tasks. 
//It has a Renderer object, and (right now) a vector of Mesh *s

//depth is -1 - 1 in GL and 0 - 1 in VK 
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED

#define cstr const char*
#define assres assert(res == VK_SUCCESS)
#define ushort uint16_t
#define uint uint32_t
#define scuint static_cast<uint32_t>


#include <arjet/renderer.h>
#include <arjet/vertex.h>
#include <arjet/mesh.h>
#include <arjet/shader.h>
#include <arjet/UBO.h>

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

class RenderController {
public:
	Renderer renderer;
	VkResult res;
	vector<Mesh*> meshes;
	RenderController(int width = 1600, int height = 900) : renderer(Renderer(width, height)) {
	}

	void init() {
		initVulkan();

		//Add shader data
		//Pass list of shaders, do this in for loop TODO
		renderer.shaders.push_back(Shader("Shaders/vert.spv", "Shaders/frag.spv", 0, renderer.device));
		renderer.shaders.push_back(Shader("Shaders/lightV.spv", "Shaders/lightF.spv", 1, renderer.device));

		layThePipe();

		//Add meshes

		//finalizeVulkan();

		//
	}

	void initVulkan() { //Part one of setting up vulkan. Create shaders after calling this function
		renderer.initWindow();
		renderer.initInstance();
		renderer.createSurface();
		renderer.findPhysicalDevice();
		renderer.createLogicalDevice();
		renderer.createSwapchain();
		renderer.createRenderPass();
		renderer.createDescriptorSetLayout();
	}
	void layThePipe() {//Yes. I named it that. I have fallen that far. 
			//Anyway this is called after shaders are created, but before meshes
		renderer.createPipeline();
		renderer.createCommandPool();
		renderer.createDepthResources();
		renderer.createFramebuffers();
		renderer.createTextureSampler();
		renderer.createDescriptorPool();
	}
	void finalizeVulkan() { //I need to have the descriptor sets from the objects ready before I can process these functions
		createCommandBuffers(); 
		renderer.createSyncObjects();
	}


	void createCommandBuffers() {
		renderer.commandBuffers.resize(renderer.swapchainFramebuffers.size());
		assert(renderer.commandBuffers.size() > 0);

		VkCommandBufferAllocateInfo cmdInfo = {};
		cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdInfo.pNext = NULL;
		cmdInfo.commandPool = renderer.commandPool;
		cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdInfo.commandBufferCount = (UINT)renderer.commandBuffers.size();
		res = vkAllocateCommandBuffers(renderer.device, &cmdInfo, renderer.commandBuffers.data());

		assres;

		for (UINT i = 0; i < renderer.commandBuffers.size(); i++) {

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			res = vkBeginCommandBuffer(renderer.commandBuffers[i], &beginInfo);
			assres;

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderer.renderPass;
			renderPassInfo.framebuffer = renderer.swapchainFramebuffers[i];
			renderPassInfo.renderArea.offset = { 0,0 };
			renderPassInfo.renderArea.extent = renderer.swapchainExtent;

			std::array<VkClearValue, 2> clearValues = {  };
			clearValues[0].color = { 0.3f, 0.3f, 0.3f, 1.0f };
			clearValues[1].depthStencil = { 1.0f, 0 };
			renderPassInfo.clearValueCount = scuint(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();
			vkCmdBeginRenderPass(renderer.commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			cout << "Shaders.size = " << renderer.shaders.size() << endl;

			for (int j = 0; j < renderer.shaders.size(); j++) {	
				cout << "Checking shader " << j << endl;
				vkCmdBindPipeline(renderer.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.graphicsPipelines[j]);
				for (int k = 0; k < meshes.size(); k++) { //For each mesh
					cout << "Meshes[" << k << "].shaderIndex = " << meshes[k]->shaderIndex << endl;

					if (meshes[k]->shaderIndex == j) { //Is this mesh set to this shader?
						cout << "Drawing mesh " << k << " and shader " << j << endl;
						VkDeviceSize offsets[] = { 0 };
						VkBuffer vertexBufferArray[] = { meshes[k]->vertexBuffer };
						vkCmdBindVertexBuffers(renderer.commandBuffers[i], 0, 1, vertexBufferArray, offsets);
						vkCmdBindIndexBuffer(renderer.commandBuffers[i], meshes[k]->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
						vkCmdBindDescriptorSets(renderer.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipelineLayout, 0, 1, &meshes[k]->descriptorSets[i], 0, NULL);
						vkCmdDrawIndexed(renderer.commandBuffers[i], meshes[k]->indicesSize, 1, 0, 0, 0);

					}
				}
			}
			vkCmdEndRenderPass(renderer.commandBuffers[i]);
			res = vkEndCommandBuffer(renderer.commandBuffers[i]);
			assres;
		}
	}
};