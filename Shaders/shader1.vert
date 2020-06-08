#version 450
#extension GL_ARB_separate_shader_objects : enable
//Basic vertex shader
//Passes in world space
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTan;
layout(location = 4) in vec3 inbit;

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
	uint featureFlags;
} ubo;


layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec3 fragPos; //in world space



void main(){
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
	fragTexCoord = inTexCoord;
	normal = ubo.normalMatrix * inNormal;
	fragPos = vec3(ubo.model * vec4(inPosition, 1.0));
}