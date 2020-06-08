#version 450
#extension GL_ARB_separate_shader_objects : enable

const uint ARJET_SHADER_FLAG_NORMAL = 1;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in mat3 TBN;


layout(binding = 1) uniform sampler2D[2] texSampler;

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


layout(location = 0) out vec4 outColor;

void main(){
	outColor = vec4(0,0,0,0);
	//outColor += texture(texSampler[0], fragTexCoord) * 0.3; //ambiant light
	outColor += texture(texSampler[0], fragTexCoord) * 0.8; //ambiant light
	
	vec3 norm;
	//Normal mapping stuff
	if ((ubo.featureFlags & ARJET_SHADER_FLAG_NORMAL) != 0){ //Has normal map
		vec3 nvec = vec3(texture(texSampler[1], fragTexCoord).xyz);
		nvec = normalize(nvec *2 - 1.0);
		norm = normalize(TBN * nvec);
	} 
	else{
		norm = normalize(normal);
	}


	//Diffuse lighting from point light
	vec3 lightDir = normalize(ubo.infos[0].position - fragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	outColor += texture(texSampler[0], fragTexCoord) * diff*3;
	
}