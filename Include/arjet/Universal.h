#pragma once
//This class is, ultimately, the engine. Its name is a legacy. 
//It is the ultimate object, and holds all the properties of the game




#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL.h>
#include <iostream>
#include <vector>

#include <arjet/renderer.h>
#include <arjet/model.h>
#include <arjet/input.h>
#include <arjet/camera.h>
#include <arjet/time.h>
#include <arjet/GameObject.h>
#include <arjet/SceneLoader.h>



#define GLM_FORCE_DEPTH_ZERO_TO_ONE //depth is -1 - 1 in GL and 0 - 1 in VK 
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES //Doesn't work if I don't do this. Alignment is still mostly a mystery to me.


#define cstr const char*
#define assres assert(res == VK_SUCCESS)
#define ushort uint16_t
#define uint uint32_t
#define ulong uint64_t
#define scuint static_cast<uint32_t>
#define ARJET_SHADER_FLAG_NORMAL 1 // flag for telling my shaders this mesh uses normal maps

typedef std::unique_ptr<GameObject> GameObjectPtr;
using glm::mat4;

using std::vector;
using std::cout;
using std::endl;


class Universal{
public:
	static mat4 viewMatrix; //reference to the main camera's view matrix
	static Renderer renderer;
	static vector<GameObjectPtr> gameObjects;
	static Input input;
	static Camera* mainCamera;

	static bool mouseMode; //keeps track of if I should trap the mouse

	static int run();//Executes everything. Main() only points here.

	static void mainLoop(); //Master perframe loop
};