#pragma once
//This class holds a lot of information that other classes may need access to
//Pointers to commonly used things


#include <glm/glm.hpp>
#include <arjet/renderer.h>
#include <arjet/GameObject.h>
#include <arjet/input.h>

using glm::mat4;

class Universal{
public:
	static mat4* viewMatrix; //reference to the main camera's view matrix
	static Renderer* renderer;
	static vector<GameObject>* gameObjects;
	static Input* input;
};
