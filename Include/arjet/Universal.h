#pragma once
//This class holds a lot of information that other classes may need access to

#include <glm/glm.hpp>

using glm::mat4;

class Universal{
public:
	static mat4* viewMatrix; //reference to the main camera's view matrix
};
