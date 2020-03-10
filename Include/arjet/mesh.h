#ifndef MESH_H
#define MESH_H
//adapted from learnopengl, mostly my own
//Differs from original in that instead of setting up the draw calls and then making the draw call,
//it sends a command buffer to the renderer and then the renderer executes all command buffers
//This doesn't allow for occulsion, but we'll cross that bridge when we come to it.

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
	unsigned int id;
	string type;
	string path;
};

class Mesh {
public:
	//Data
	Renderer& renderer;
	vector<Vertex> vertices;
	vector<unsigned int> indices;
	vector<Texture> textures;


	VkBuffer vertexBuffer;
	//Functions
	Mesh(Renderer& renderer, vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures) : renderer(renderer){
		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;
	}
	void createVertexBuffer() {}
	void createDescriptorSet() {}//TODO

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
//		}
//		glActiveTexture(GL_TEXTURE0);
//
//		//draw
//		glBindVertexArray(VAO);
//		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
//		glBindVertexArray(0);
//	}
//private:
//	unsigned int VAO, VBO, EBO;
//	void setupMesh() {
//		glGenVertexArrays(1, &VAO);
//		glGenBuffers(1, &VBO);
//		glGenBuffers(1, &EBO);
//
//		glBindVertexArray(VAO);
//		glBindBuffer(GL_ARRAY_BUFFER, VBO);
//		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
//
//		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
//		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
//
//		//vertex positions
//		glEnableVertexAttribArray(0);
//		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
//		//vertex normals
//		glEnableVertexAttribArray(1);
//		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
//		//Tex Coords
//		glEnableVertexAttribArray(2);
//		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
//		//Tangent and bitangent
//		glEnableVertexAttribArray(3);
//		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
//		glEnableVertexAttribArray(4);
//		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
//
//
//		glBindVertexArray(0);
//	}
};

#endif