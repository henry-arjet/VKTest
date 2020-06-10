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
		renderer.createStartCommands();
		renderer.createSubmitCommands();
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


	void batch() { //fills the descriptor sets, vertex buffer, and index buffer vectors. Allows for batched drawcalls
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
	}

	void createCommandBuffers() {

		uint size = renderer.shaderIndices.size(); //how many shaders (and thus pipelines) we're working with
		uint cframe = renderer.currentFrame;
		
		renderer.commandBuffers.resize(2); //match frames in flight
		for (int i = 0; i < 2; i++) {//match shaders
			renderer.commandBuffers[i].resize(size);
		}

		VkCommandBufferAllocateInfo cmdInfo = {};
			cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmdInfo.pNext = NULL;
			cmdInfo.commandPool = renderer.commandPool;
			cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cmdInfo.commandBufferCount = size;
			res = vkAllocateCommandBuffers(renderer.device, &cmdInfo, renderer.commandBuffers[cframe].data());

			for (int i = 0; i < size /*of the shaders*/; i++) {
				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				res = vkBeginCommandBuffer(renderer.commandBuffers[cframe][i], &beginInfo);
				assres;

				vkCmdBindPipeline(renderer.commandBuffers[cframe][i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.graphicsPipelines[i]);

				vector<VkDeviceSize> offsets = {}; //creates a vector of offsets that are all 0
				for (int l = 0; l < vertexBuffers[i].size(); l++) {
					offsets.push_back(0);
				}
				vkCmdBindVertexBuffers(renderer.commandBuffers[cframe][i], 0, vertexBuffers[i].size(), vertexBuffers[i].data(), offsets.data());
				vkCmdBindDescriptorSets(renderer.commandBuffers[cframe][i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipelineLayout, 0, descriptorSets[i][cframe].size(), descriptorSets[i][cframe].data(), 0, NULL);
				for (int j = 0; j < indexBuffers.size(); j++) { //for each draw call
					vkCmdBindIndexBuffer(renderer.commandBuffers[cframe][i], indexBuffers[i][j], 0, VK_INDEX_TYPE_UINT32);
					vkCmdDrawIndexed(renderer.commandBuffers[cframe][i], indexSizes[i][j], 1, 0, 0, 0);
				}
				res = vkEndCommandBuffer(renderer.commandBuffers[cframe][i]);
				assres;
			}
		}

	/*VkCommandBuffer * createCommandBuffersModel(vector<Mesh> modelMeshes) { //creates the draw calls for a model
		//Should return an array of size 2, one buffer for each frame in flight
		VkCommandBuffer buffers[2];

		VkCommandBufferAllocateInfo cmdInfo = {};
		cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdInfo.pNext = NULL;
		cmdInfo.commandPool = renderer.commandPool;
		cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdInfo.commandBufferCount = 2;
		res = vkAllocateCommandBuffers(renderer.device, &cmdInfo, buffers);

		assres;

		for (UINT i = 0; i < 2; i++) { //hardcoded to create two command buffers

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			res = vkBeginCommandBuffer(buffers[i], &beginInfo);
			assres;

			for (int k = 0; k < modelMeshes.size(); k++) { //For each mesh
				VkDeviceSize offsets[] = { 0 };
				VkBuffer vertexBufferArray[] = { modelMeshes[k].vertexBuffer };
				vkCmdBindVertexBuffers(buffers[i], 0, 1, vertexBufferArray, offsets);
				vkCmdBindIndexBuffer(buffers[i], modelMeshes[k].indexBuffer, 0, VK_INDEX_TYPE_UINT32);
				vkCmdBindDescriptorSets(buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipelineLayout, 0, 1, &modelMeshes[k].descriptorSets[i], 0, NULL);
				vkCmdDrawIndexed(buffers[i], modelMeshes[k].indicesSize, 1, 0, 0, 0);

			}
			vkCmdEndRenderPass(renderer.commandBuffers[i]);
			res = vkEndCommandBuffer(renderer.commandBuffers[i]);
			assres;
		}
		return buffers;
	}*/

	void finishFrame() { //god this is jank
		batch();
		createCommandBuffers();

		vector<VkCommandBuffer> finalBuffers;
		finalBuffers.push_back(renderer.startCommandBuffers[renderer.currentFrame]);

		for (int i = 0; i < renderer.commandBuffers.size(); i++) {
			finalBuffers.push_back(renderer.commandBuffers[renderer.currentFrame][i]);
		}
		finalBuffers.push_back(renderer.submitCommandBuffers[renderer.currentFrame]);//Both should be the same, should just have a "submit" buffer ready to go
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkSemaphore waitSemaphores[] = { renderer.imageAvailableSemaphores[renderer.currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = finalBuffers.size();
		submitInfo.pCommandBuffers = finalBuffers.data();
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