#pragma once
//adapted partly from vulken-tutorial, mostly my own

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <arjet/vertex.h>
#include <arjet/renderer.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

using std::vector;
using std::string;
using glm::vec3;
using glm::vec2;


struct Texture {
	unsigned int texIndex = 0;
	string type = "blank";
	string path = "blank"; //At the moment, only used to check if it's been loaded yet
};

class Mesh {
public:
	//Data
	Renderer& renderer;
	vector<Texture> textures; //Nevermind it is
	uint index = 0; //This is how it will keep track of where it is in the arrays in renderer
	uint texIndex = 0; //Not used?
	uint shaderIndex = 0;
	VkDescriptorSet descriptorSets[2]; //Local. One for each frame
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	UniformBufferObject ubo;
	vector<VkBuffer> uniformBuffers;
	vector<VkDeviceMemory> uniformBuffersMemory;
	vec3 position; //Should be in object, but yolo
	vec3 scale = vec3(0.3f, 0.3f, 0.3f);
	uint indicesSize = 3; //This should not have to exist, but function calls from pointer don't seem to be working
	uint featureFlags = 0; //Tells the shaders which features it supports. E.g. normal maps
	//Functions

	Mesh(Renderer& renderer) : renderer(renderer) {}

	Mesh(Renderer& renderer, vector<Vertex> vertices, vector<uint> indices, vector<Texture> textures, uint index, uint featureFlags = 0) : renderer(renderer) {
		this->index = index;
		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;
		this->featureFlags = featureFlags;
	}



	void init() {
		createVertexBuffer();
		createIndexBuffer();
		createUniformBuffers();
		createDescriptorSets();
		//pushMesh();
		indicesSize = indices.size(); //Again, shouldn't exist
	}

	void createVertexBuffer() {//creates a VK vertex buffer from the vertices data it has
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		renderer.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(renderer.device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(renderer.device, stagingBufferMemory);

		renderer.createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
		renderer.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vkDestroyBuffer(renderer.device, stagingBuffer, NULL);
		vkFreeMemory(renderer.device, stagingBufferMemory, NULL);
	}

	void createIndexBuffer() {
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		renderer.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;

		vkMapMemory(renderer.device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vkUnmapMemory(renderer.device, stagingBufferMemory);

		renderer.createBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
		renderer.copyBuffer(stagingBuffer, indexBuffer, bufferSize);

		vkDestroyBuffer(renderer.device, stagingBuffer, NULL);
		vkFreeMemory(renderer.device, stagingBufferMemory, NULL);

	}

	void createUniformBuffers() {
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);
		size_t s = renderer.swapchainImages.size(); //might save a few cycles by not having to look up sci.size() thrice. Might not. IDK.
		uniformBuffers.resize(s);
		uniformBuffersMemory.resize(s);
		for (size_t i = 0; i < s; i++) {
			renderer.createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
				| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
		}
	}

	void createDescriptorSets() {
		size_t s = renderer.swapchainImages.size();//number of frames
		vector<VkDescriptorSetLayout> layouts(s, renderer.descriptorSetLayout); //Assumes only one layout type
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.descriptorPool = renderer.descriptorPools[index];
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorSetCount = static_cast<uint>(s);
		allocInfo.pSetLayouts = layouts.data();

		//descriptorSets.resize(s); //Local. One for each frame
		VkResult res = vkAllocateDescriptorSets(renderer.device, &allocInfo, descriptorSets);
		assres;

		for (uint i = 0; i < s; i++) {
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1; //I could potentially write the descriptors for all objects at once. That can be done later.
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			//texture
			std::array<VkDescriptorImageInfo, 2> imageInfo = {};

			imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo[0].imageView = renderer.textureImageViews[textures[0].texIndex]; //Should do it like this so I don't have copies of the same texture. 
			imageInfo[0].sampler = renderer.textureSampler; //the default texture sampler we set up in the Renderer class

			imageInfo[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			if ((featureFlags & ARJET_SHADER_FLAG_NORMAL) != 0) {// checks if mesh has normal
				imageInfo[1].imageView = renderer.textureImageViews[textures[1].texIndex]; //This one is the normal map
				cout << "TEST!\n";
			}
			else imageInfo[1].imageView = renderer.textureImageViews[0]; //else hands it whatever is texture 0. Doesn't matter
			imageInfo[1].sampler = renderer.textureSampler;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 2;
			descriptorWrites[1].pImageInfo = imageInfo.data();

			vkUpdateDescriptorSets(renderer.device, static_cast<uint>(descriptorWrites.size()), descriptorWrites.data(), 0, NULL);
		}
	}
	void updateUniformBuffer(uint currentImage) {
		static auto startTime = std::chrono::high_resolution_clock::now();

		//auto currentTime = std::chrono::high_resolution_clock::now();
		//float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		ubo.model = glm::translate(mat4(1.0f), position);
		//ubo.model = glm::rotate(mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));//Translate then rotate
		ubo.model = glm::scale(ubo.model, scale);

		ubo.proj = glm::perspective(glm::radians(70.0f), renderer.swapchainExtent.width / (float)renderer.swapchainExtent.height, 0.02f, 100.0f); //TODO move to init function
		ubo.proj[1][1] *= -1;

		ubo.normalMatrix = mat3(glm::transpose(glm::inverse(ubo.model)));

		ubo.featureFlags = featureFlags; //TODO move to init

		void* data;
		vkMapMemory(renderer.device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));

		vkUnmapMemory(renderer.device, uniformBuffersMemory[currentImage]);
	}
	void pushMesh() {
		if (renderer.descriptorSets.size() <= index) {
			renderer.descriptorSets.resize(index + 1);
		}
		//renderer.descriptorSets[index] = descriptorSets;

		if (renderer.vertexBuffers.size() <= index) {
			renderer.vertexBuffers.resize(index + 1);
		}
		renderer.vertexBuffers[index] = vertexBuffer;

		if (renderer.indexBuffers.size() <= index) {
			renderer.indexBuffers.resize(index + 1);
		}
		renderer.indexBuffers[index] = indexBuffer;

		if (renderer.indicesSize.size() <= index) {
			renderer.indicesSize.resize(index + 1);
		}
		renderer.indicesSize[index] = indices.size();

		if (renderer.shaderIndices.size() <= index) {
			renderer.shaderIndices.resize(index + 1);
		}renderer.shaderIndices[index] = shaderIndex;

	}
	vector<uint> indices;
private:
	vector<Vertex> vertices;
};