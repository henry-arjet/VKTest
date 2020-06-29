#include <arjet/mesh.h>
#include <arjet/GameObject.h>

void Mesh::createVertexBuffer() {//creates a VK vertex buffer from the vertices data it has
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

void Mesh::createIndexBuffer() {
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

void Mesh::createUniformBuffers() {
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	size_t s = renderer.swapchainImages.size();
	uniformBuffers.resize(s);
	uniformBuffersMemory.resize(s);
	for (size_t i = 0; i < s; i++) {
		renderer.createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
	//Don't copy this to an optimal memory location because it will be updated every frame
	}
}

void Mesh::createDescriptorSets() {
	//First create a pool for the sets
	//Might want to make this a method of Renderer
	size_t s = renderer.swapchainImages.size();//number of frames
	descriptorSets.resize(s);
	std::vector<VkDescriptorPoolSize>  poolSizes(2); //UBO + texture. I should really automate this
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint>(s);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint>(s * 2);

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint>(s);
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	VkResult res = vkCreateDescriptorPool(renderer.device, &poolInfo, NULL, &descriptorPool);
	assres;

	vector<VkDescriptorSetLayout> layouts(s, renderer.descriptorSetLayout); //Assumes only one layout type
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorSetCount = static_cast<uint>(s);
	allocInfo.pSetLayouts = layouts.data();

	//descriptorSets.resize(s); //Local. One for each frame
	res = vkAllocateDescriptorSets(renderer.device, &allocInfo, descriptorSets.data());
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

void Mesh::updateUniformBuffer(uint currentFrame) {
	ubo.model = glm::translate(mat4(1.0f), daddyModel->gameObject.transform->position); //that's a bit of a mess
	ubo.model = glm::scale(ubo.model, daddyModel->gameObject.transform->size);

	ubo.view = *daddyModel->view; //If I understand c++ correctly this should just copy

	ubo.normalMatrix = mat3(glm::transpose(glm::inverse(ubo.model)));

	void* data;
	vkMapMemory(renderer.device, uniformBuffersMemory[currentFrame], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(renderer.device, uniformBuffersMemory[currentFrame]);
}

void Mesh::processUBOConstants() {
	//Put in some constants for the UBO. Can be changed later if need be
	ubo.proj = glm::perspective(glm::radians(70.0f), renderer.swapchainExtent.width / (float)renderer.swapchainExtent.height, 0.02f, 100.0f);
	ubo.proj[1][1] *= -1;//Don't really know why, but I need this
	ubo.featureFlags = featureFlags;
}
