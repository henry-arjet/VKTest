#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 fragPos;


layout(binding = 1) uniform sampler2D texSampler;

struct LightInfo{
	vec3 position;
	vec3 color;
	float strength;
	bool isDirectional;
	bool inUse;
};

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat3 normalMatrix;
	LightInfo infos[4];
} ubo;


layout(location = 0) out vec4 outColor;

void main(){
	outColor = vec4(0,0,0,0);
	outColor += texture(texSampler, fragTexCoord) * 0.3; //ambiant light
	
	//Diffuse lighting from point light
	vec3 norm = normalize(normal);
	vec3 lightDir = normalize(ubo.infos[0].position - fragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	outColor += texture(texSampler, fragTexCoord) * diff*2;

}