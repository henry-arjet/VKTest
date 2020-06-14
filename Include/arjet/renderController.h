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
#include <arjet/model.h>

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
	vector<Model*> models;

	vector<vector<vector<VkDescriptorSet>>> descriptorSets; //3d. 1st for the shader, second for the frame, third for the descriptor sets.
															//Could simplify to vector of arrays. Shouldn't it be the same for both frames?
	vector<vector<VkBuffer>> vertexBuffers; //2d, it will be the same vertex buffer for both frames
	vector<vector<VkBuffer>> indexBuffers;
	vector<vector<uint>> indexSizes;


	RenderController(int width = 1600, int height = 900) : renderer(Renderer(width, height)) {
	}

	void init() {
		initVulkan();

		//Add shader data
		//Pass list of shaders, do this in for loop TODO
		renderer.shaders.push_back(Shader("Shaders/vert.spv", "Shaders/frag.spv", 0, renderer.device));
		renderer.shaders.push_back(Shader("Shaders/lightV.spv", "Shaders/lightF.spv", 1, renderer.device));

		layThePipe();
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
	void finalizeVulkan() {
		renderer.createSyncObjects();

		//resize vectors
		//Assumes number of shaders constant
		uint s = renderer.shaderIndices.size();
		descriptorSets.resize(s);
		for (int i = 0; i < s; i++){
			descriptorSets[i].resize(2);
		}
		vertexBuffers.resize(s);
		indexBuffers.resize(s);
		indexSizes.resize(s);
	}


	/*void batch() { //fills the descriptor sets, vertex buffer, and index buffer vectors. Allows for batched drawcalls
				   //Should be called every frame
		uint cframe = renderer.currentFrame;

		for (uint i = 0; i < renderer.shaderIndices.size(); i++) {//for each shader
			cout << "Got this far" << endl;
			cout << "models.size() = " << models.size() << endl;

			for (uint j = 0; j < models.size(); j++) {
				if (models[j]->draw == true) {
					for (uint k = 0; k < models[j]->meshes.size(); k++) {
						descriptorSets[i][cframe].push_back(models[j]->meshes[k].descriptorSets[cframe]);
						vertexBuffers[i].push_back(models[j]->meshes[k].vertexBuffer);
						indexBuffers[i].push_back(models[j]->meshes[k].indexBuffer);
						indexSizes[i].push_back(models[j]->meshes[k].indicesSize);
					}
				}
			}
		}
	}*/

	void createCommandBuffers() {

		uint size = renderer.shaderIndices.size(); //how many shaders (and thus pipelines) we're working with. Depreciated
		uint cframe = renderer.currentFrame;
		

		VkCommandBufferAllocateInfo cmdInfo = {};
		cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdInfo.pNext = NULL;
		cmdInfo.commandPool = renderer.commandPool;
		cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdInfo.commandBufferCount = 1;
		res = vkAllocateCommandBuffers(renderer.device, &cmdInfo, &renderer.commandBuffers[cframe]);

		VkCommandBufferBeginInfo beginInfo = {};

		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		res = vkBeginCommandBuffer(renderer.commandBuffers[cframe], &beginInfo);
		assres;


		vector<VkCommandBuffer> secondaryBuffers;
		secondaryBuffers.reserve(models.size()); //Allocates enough space for all the models so we don't have to reallocate
		for (int i = 0; i < models.size(); i++) {
			if (models[i]->draw) { //If this model should be drawn

				secondaryBuffers.push_back(models[i]->buffers[cframe]);
			}
		}
		vkCmdExecuteCommands(renderer.commandBuffers[cframe], secondaryBuffers.size(), secondaryBuffers.data());
				
		res = vkEndCommandBuffer(renderer.commandBuffers[cframe]);
		assres;
			
	}

	void createCommandBuffersModel(vector<Mesh> modelMeshes, VkCommandBuffer buffers[2]) { //creates the draw calls for a model
											//Should return an array of size 2, one buffer for each frame in flight

		VkCommandBufferAllocateInfo cmdInfo = {};
		cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdInfo.pNext = NULL;
		cmdInfo.commandPool = renderer.commandPool;
		cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		cmdInfo.commandBufferCount = 2;
		res = vkAllocateCommandBuffers(renderer.device, &cmdInfo, buffers);

		assres;

		for (UINT i = 0; i < 2; i++) { //hardcoded to create two command buffers

			VkCommandBufferInheritanceInfo inherInfo = {};
			inherInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
			inherInfo.occlusionQueryEnable = 0;
			inherInfo.queryFlags = 0; //I have no fucking clue what inherited queries are. Or where they're from. But spec says I need this.
			inherInfo.renderPass = renderer.renderPass;
			//inherInfo.framebuffer //Could do this, might help a bit. Not required


			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; 
			//Set it up for being called from the main render pass, and called multiple times
			//I shouldn't need simultaneous as I'm creating two draw calls, but that's ineficcient
			beginInfo.pInheritanceInfo = &inherInfo;
			res = vkBeginCommandBuffer(buffers[i], &beginInfo);
			assres;
			vkCmdBindPipeline(buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.graphicsPipelines[0]); //HARDCODED TO BASE SHADER
			
			for (int k = 0; k < modelMeshes.size(); k++) { //For each mesh
				VkDeviceSize offsets[] = { 0 };
				VkBuffer vertexBufferArray[] = { modelMeshes[k].vertexBuffer };
				vkCmdBindVertexBuffers(buffers[i], 0, 1, vertexBufferArray, offsets);
				vkCmdBindIndexBuffer(buffers[i], modelMeshes[k].indexBuffer, 0, VK_INDEX_TYPE_UINT32);
				vkCmdBindDescriptorSets(buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipelineLayout, 0, 1, &modelMeshes[k].descriptorSets[i], 0, NULL);
				vkCmdDrawIndexed(buffers[i], modelMeshes[k].indicesSize, 1, 0, 0, 0);
			}
			res = vkEndCommandBuffer(buffers[i]);
			assres;
		}
	}

	void finishFrame() { //god this is jank
		createCommandBuffers();
		//Create the final command buffer
		uint cframe = renderer.currentFrame;


		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkSemaphore waitSemaphores[] = { renderer.imageAvailableSemaphores[renderer.currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &renderer.commandBuffers[cframe];
		VkSemaphore signalSemaphores[] = { renderer.renderFinishedSemaphores[renderer.currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;
		vkResetFences(renderer.device, 1, &renderer.inFlightFences[renderer.currentFrame]);
		res = vkQueueSubmit(renderer.graphicsQueue, 1, &submitInfo, renderer.inFlightFences[renderer.currentFrame]);
		assres;

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		VkSwapchainKHR swapchains[] = { renderer.swapchain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = &renderer.imageIndex;

		res = vkQueuePresentKHR(renderer.graphicsQueue, &presentInfo);//remember we asserted that graphicsQueue = presentQueue
		if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || renderer.framebufferResized) {
			renderer.framebufferResized = false;
			//recreateSwapchain(); TODO
			return;
		}
		else if (res != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image");
		}
		renderer.currentFrame = (renderer.currentFrame + 1) % renderer.MAX_FRAMES_IN_FLIGHT;
	}
};