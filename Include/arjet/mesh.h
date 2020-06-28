#pragma once

//adapted partly from vulken-tutorial, mostly my own

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <arjet/vertex.h>
#include <arjet/renderer.h>
#include <arjet/UBO.h>
#include <arjet/model.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

using std::vector;
using std::string;
using glm::vec3;
using glm::vec2;

class Renderer; //Renderer and Mesh reference eachother, so I need forward dec
class Model;

struct Texture {
	unsigned int texIndex = 0;
	string type = "blank";
	string path = "blank"; //At the moment, only used to check if it's been loaded yet
};

class Mesh{
public:
	Renderer& renderer;
	vector<Texture> textures; //different ones for diffuse, spec, normal etc
	VkBuffer vertexBuffer;
	VkBuffer indexBuffer;
	UniformBufferObject ubo;
	vector<VkBuffer> uniformBuffers; //one per frame
	Model* daddyModel; //The parent model to which this mesh is attached 


	
	uint featureFlags = 0; //Tells the shaders which features it supports. E.g. normal maps
	uint shaderIndex; //Which shader we're using. I should probably change this to a path like I did with textures. Lots can be done later.


	vector<VkDescriptorSet> descriptorSets; //One per each frame in flight
	VkDescriptorPool descriptorPool; //Yes, I probably should batch these all together. But that is not my priority right now


	Mesh(Renderer& renderer, vector<Vertex> vertices, vector<uint> indices, vector<Texture> textures, /*uint index,*/ uint featureFlags, Model* parent) : renderer(renderer), daddyModel(parent) {
		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;
		this->featureFlags = featureFlags;

		createVertexBuffer();
		createIndexBuffer();
		createUniformBuffers();
		createDescriptorSets();
		processUBOConstants();

	}

	uint getIndicesSize() {
		if (indicesSize == UINT32_MAX) {
			indicesSize = indices.size();
		}
		return indicesSize;
	}

	void updateUniformBuffer(uint currentImage);//Updates the uniform buffer currently about to be drawn
private:
	uint indicesSize = UINT32_MAX;
	VkDeviceMemory vertexBufferMemory;
	VkDeviceMemory indexBufferMemory;
	vector<VkDeviceMemory> uniformBuffersMemory;


	vector<Vertex> vertices;
	vector<uint>indices;

	void createVertexBuffer();

	void createIndexBuffer();

	void createUniformBuffers();

	void createDescriptorSets(); //I might want to move this over to the model.
	//The only problem is the shader flags which will be unique to each mesh. I don't want
	//to tell the shader that every mesh has a normal and spec when only one does.
	//Could make a model descriptor set and a mesh descriptor set, but I don't think I will

	void processUBOConstants();

};

