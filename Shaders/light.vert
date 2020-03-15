#version 450
#extension GL_ARB_separate_shader_objects : enable
//I could optimize this by not using a texture or normals/texcord/tbn
//But that would be far more trouble than it's worth
layout(location = 0) in vec3 inPosition;

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

void main(){
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
}