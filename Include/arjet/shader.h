#ifndef SHADER_H
#define SHADER_H

#include <vulkan/vulkan.hpp>


#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

using std::string;
using std::cout;
using std::endl;
using std::ifstream;
using std::vector;

using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;


static vector<char> readFile(const std::string& filename) { //Copied from renderer, in turn copied from Vulkan Tutorial
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

class Shader {
public:
	VkPipelineShaderStageCreateInfo shaderStages[2];
	uint index;//Just to keep track of it. Should match the index in the renderer
	Shader(const string vertexPath, const string fragmentPath, uint index, VkDevice d) {
		device = d;
		this->index = index;
		//read from files
		string vertexCode;
		string fragmentCode;
		ifstream vShaderFile;
		ifstream fShaderFile;
		uint32_t shaderIndex;
		auto vertShaderCode = readFile(vertexPath.c_str());
		auto fragShaderCode = readFile(fragmentPath.c_str());

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

		shaderStages[0] = vertShaderStageInfo;
		shaderStages[1] = fragShaderStageInfo;
	}
private:
	VkDevice device;
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
};

#endif