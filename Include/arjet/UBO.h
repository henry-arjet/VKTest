#pragma once
//Contains the LightInfo and Uniform Buffer Object structs.
//Used by renderer and mesh
//Any modifications made here must also be made in the shaders
//Sadly, you cant #include external c code in GLSL
	//Although there should be a way to. I might research it
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using glm::vec3;
using glm::mat3;
using glm::mat4;


struct LightInfo {
	alignas(16) vec3 position;
	alignas(16) vec3 color = vec3(1.0f, 1.0f, 1.0f);
	alignas(4)  float strength = 1.0f;
	alignas(4)  bool isDirectional = false;
	alignas(4)  bool inUse = false;

};


struct UniformBufferObject {
	alignas(16) mat4 model;
	alignas(16) mat4 view;
	alignas(16) mat4 proj;
	alignas(16) mat3 normalMatrix;
	alignas(16) LightInfo lights[4]; //have to hard code this. At least for now
};