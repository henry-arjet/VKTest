#version 450
#extension GL_ARB_separate_shader_objects : enable

const uint ARJET_SHADER_FLAG_NORMAL = 1;
const uint ARJET_SHADER_FLAG_SPECULAR = 2;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in mat3 TBN;


layout(binding = 1) uniform sampler2D[3] texSampler;

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
	mat4 normalMatrix;
	vec3 viewPos;
	uint featureFlags;
	
	
	LightInfo infos[4];
} ubo;


layout(location = 0) out vec4 outColor;

void main(){
	outColor = vec4(0,0,0,0);
	outColor += texture(texSampler[0], fragTexCoord) * 0.3; //ambiant light
	//outColor += texture(texSampler[0], fragTexCoord) * 0.8; //ambiant light
	
	vec3 norm;
	//Normal mapping stuff
	if ((ubo.featureFlags & ARJET_SHADER_FLAG_NORMAL) != 0){ //Has normal map
		vec3 nvec = vec3(texture(texSampler[2], fragTexCoord).xyz);
		nvec = normalize(nvec *2 - 1.0);
		norm = normalize(TBN * nvec);
	} 
	else{
		norm = normalize(normal);
	}

	

	//Diffuse lighting from point light
	vec3 lightDir = normalize(ubo.infos[0].position - fragPos );
	float diff = max(dot(norm, lightDir), 0.0);
	outColor += texture(texSampler[0], fragTexCoord) * diff*3;

	//specular
	float sStrength = 0.5f;
	vec3 viewDir = normalize(ubo.viewPos-fragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32) * sStrength;
	//outColor += vec4((reflectDir+1)*0.5, 0.0);
	//outColor += vec4((norm+1)*.5, 0.0);
	outColor += vec4(spec,spec,spec, 0.0);
	

	//specular map
	/*if ((ubo.featureFlags & ARJET_SHADER_FLAG_SPECULAR) != 0){ //Has specular map
		vec3 svec = vec3(texture(texSampler[1], fragTexCoord).xyz);
		vec3 viewDir = normalize(-fragPos);
		vec3 reflectDir = reflect(-lightDir, norm);
		vec3 spec = pow(max(dot(viewDir, reflectDir), 0.0), 32) * svec;
		outColor += vec4(spec, 0.0);
	}*/
	
}