#ifndef MESH_H
#define MESH_H
//adapted partly from vulken-tutorial, mostly my own
//TODO fix this. Make it a part of a renderer class, like Shader.

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
	vector<Texture> textures; //Not used right now
	uint index = 0; //This is how it will keep track of where it is in the arrays in renderer
	uint texIndex = 0;
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
	//Functions
	
	Mesh(Renderer& renderer) : renderer(renderer) {}
	
	Mesh(Renderer& renderer, vector<Vertex> vertices, vector<uint> indices, vector<Texture> textures, uint index) : renderer(renderer){
		this->index = index;
		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;
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
			VkDescriptorImageInfo imageInfo = {};

			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			//cout << "Applying texture from " << textures[0].path << endl;
			imageInfo.imageView = renderer.textureImageViews[textures[0].texIndex ]; //Should do it like this so I don't have copies of the same texture. 
			imageInfo.sampler = renderer.textureSampler; //the default texture sampler we set up in the Renderer class

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;

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
		void* data;


		vkMapMemory(renderer.device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		cout << "Size of ubo: " << sizeof(ubo) << endl;

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

	vector<uint> indices = { //must be public so I can get the size of it
		0,  1,  2,  2,  3,  0,
		4,  5,  6,  6,  7,  4,
		8,  9,  10, 10, 11, 8,
		12, 13, 14, 14, 15, 12,
		16, 17, 18, 18, 19, 16,
		20, 21, 22, 22, 23, 20,
	};
	private:
		vector<Vertex> vertices = {
			{{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}}, //0  bl  FRONT
			{{ 0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, //1  br
			{{ 0.5f,  0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, //2  tr
			{{-0.5f,  0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}, //3  tl
			
		
			{{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}}, //4     TOP
			{{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}, //5
			{{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, //6
			{{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, //7


			{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}}, //8  bl  BACK
			{{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}}, //9  br
			{{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}}, //10 tr
			{{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}}, //11 tl


			{{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}}, //12     TOP
			{{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}}, //13
			{{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}}, //14
			{{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}}, //15


			{{ 0.5f, -0.5f,  0.5f}, {1.0f,  0.0f, 0.0f}, {1.0f, 1.0f}}, //16     Right
			{{ 0.5f, -0.5f, -0.5f}, {1.0f,  0.0f, 0.0f}, {0.0f, 1.0f}}, //17
			{{ 0.5f,  0.5f, -0.5f}, {1.0f,  0.0f, 0.0f}, {0.0f, 0.0f}}, //18
			{{ 0.5f,  0.5f,  0.5f}, {1.0f,  0.0f, 0.0f}, {1.0f, 0.0f}}, //19

			{{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f, 0.0f}, {1.0f, 1.0f}}, //20     Right
			{{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f, 0.0f}, {0.0f, 1.0f}}, //21
			{{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f, 0.0f}, {0.0f, 0.0f}}, //22
			{{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f, 0.0f}, {1.0f, 0.0f}}, //23

		};

	//VkCommandBuffer localBuffer; //command buffer, Should just need one as the per frame commands will be handled by renderer

	//void CreateBuffer() { //creates a buffer to be pushed to the renderer 

	//	VkCommandBufferAllocateInfo cmdInfo = {};
	//	cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	//	cmdInfo.pNext = NULL;
	//	cmdInfo.commandPool = renderer.commandPool; //Might need to change this to a local command pool
	//	cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	//	cmdInfo.commandBufferCount = 1;
	//	VkResult res = vkAllocateCommandBuffers(renderer.device, &cmdInfo, &localBuffer); //ALLOCATE
	//	assres;

	//	//now that we've allocated, time to write the cmd buffers
	//	VkCommandBufferBeginInfo beginInfo = {};
	//	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	//	res = vkBeginCommandBuffer(localBuffer, &beginInfo); //BEGIN
	//	assres;

	//		/*VkRenderPassBeginInfo renderPassInfo = {}; I don't think I need to create a new render pass
	//		But this does mean I'll have to start and finish the draw call in renderer

	//		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	//		renderPassInfo.renderPass = renderer.renderPass;
	//		renderPassInfo.framebuffer = renderer.swapchainFramebuffers[i];
	//		renderPassInfo.renderArea.offset = { 0,0 };
	//		renderPassInfo.renderArea.extent = renderer.swapchainExtent;*/
	//		/*
	//		std::array<VkClearValue, 2> clearValues = {  };
	//		clearValues[0].color = { 0.3f, 0.3f, 0.3f, 1.0f };
	//		clearValues[1].depthStencil = { 1.0f, 0 };
	//		renderPassInfo.clearValueCount = scuint(clearValues.size());
	//		renderPassInfo.pClearValues = clearValues.data();
	//		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	//		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
	//		*/

	//	//Skiping up to the actual needed stuff
	//	VkBuffer vertexBuffers[] = { vertexBuffer };
	//	VkDeviceSize offsets[] = { 0 };
	//	vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
	//	vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);
	//	vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, NULL);
	//	vkCmdDrawIndexed(commandBuffers[i], static_cast<UINT>(indices.size()), 1, 0, 0, 0);
	//	vkCmdEndRenderPass(commandBuffers[i]);
	//	
	//	res = vkEndCommandBuffer(commandBuffers[i]);
	//	assres;
	//	}
	//}
//	void Draw() { All this code only really applies to GL. I'll keep it around in case I need to reference it.
//		unsigned int diffuseNr = 1;
//		unsigned int specularNr = 1;
//		unsigned int normalNr = 1;
//		//shader.setBool("material.hasNormal", false);
//		//shader.setBool("material.hasSpecular", false);
//		for (unsigned int i = 0; i < textures.size(); i++) {
//			glActiveTexture(GL_TEXTURE0 + i); //activates the right texture unit
//			//retrieve texture number
//			string number;
//			string name = textures[i].type;
//			if (name == "texture_diffuse") {
//				number = std::to_string(diffuseNr++);
//				//cout << "Setting diffuse texture\n";
//			}else if (name == "texture_specular") {
//				number = std::to_string(specularNr++);
//				//cout << "Setting specular texture\n";
//				shader.setBool("material.hasSpecular", true);
//			}
//			else if (name == "texture_normal") {
//				number = std::to_string(normalNr++);
//				shader.setBool("material.hasNormal", true);
//				//cout << "setting normal map\n";
//			}	else 
//					cout << "Unexpected texture\n";
//
//			shader.setInt(("material." + name + number).c_str(), i);
//			glBindTexture(GL_TEXTURE_2D, textures[i].id);

};

#endif