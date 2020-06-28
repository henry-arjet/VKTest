//Main sorce file
//Henry Arjet, 2020
// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED

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

//#define STB_IMAGE_IMPLEMENTATION
//#include <stb_image.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE//depth is -1 - 1 in GL and 0 - 1 in VK 
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES //Doesn't work if I don't do this. Alignment is still mostly a mystery to me.
//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>




#define cstr const char*
#define assres assert(res == VK_SUCCESS)
#define ushort uint16_t
#define uint uint32_t
#define ulong uint64_t
#define scuint static_cast<uint32_t>
#define ARJET_SHADER_FLAG_NORMAL 1 // flag for telling my shaders this mesh uses normal maps



//using glm::mat4;
//using glm::mat3;
//using glm::vec4;
//using glm::vec3;
//using glm::vec2;

using std::vector;
using std::cout;
using std::endl;

bool mouseMode = true; //keeps track of if I should trap the mouse

Camera mainCamera;

Input input;

void mainLoop(Renderer &renderer);

int main() {
	vector<ShaderPath> shaderPaths;
	shaderPaths.push_back(ShaderPath("Shaders/vert.spv", "Shaders/frag.spv"));
	shaderPaths.push_back(ShaderPath("Shaders/lightV.spv", "Shaders/lightF.spv"));

	Renderer renderer = Renderer(shaderPaths);
	
	uint tCount = 0; //keeps track of textures
	Model testModel = Model(renderer, "models/nanosuit/scene.fbx", tCount);

	renderer.models.push_back(&testModel);
	
	mainCamera = Camera();

	Time::start();
	mainLoop(renderer);
	//Should clean up the rendererer here
	return 0;
}

void mainLoop(Renderer &renderer) {
bool stillRunning = true;
while (stillRunning) {
	//deltaTime and now. Might want to make these part of a time class
	Time::resetDelta();

	double swt;
	Time::startStopwatch();
	//SDL Input
	SDL_Event event;
	uint eventCount = 0; //FOR DEBUG
	while (SDL_PollEvent(&event)) {

		swt = Time::endStopwatch();
		if (swt > 0.01) {
			cout << "Pollevents Internal time: " << swt << endl;
			cout << "Event count :" << eventCount << endl;
		}
		Time::startStopwatch();

		++eventCount;
		switch (event.type) {
		case SDL_QUIT:
			stillRunning = false;
			break;
		case SDL_WINDOWEVENT:
			switch (event.window.event) {
			case SDL_WINDOWEVENT_RESIZED:
				renderer.framebufferResized = true; //Does nothing right now. Not a priority
				break;
			default:
				break;
			}
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			input.ProcessKey(&event.key);
			break;
		default:
			break;
		}
		swt = Time::endStopwatch();
		if (swt > 0.01) {
			cout << "switchboard time: " << swt << endl;
			cout << "Event count :" << eventCount << endl;
		}
		Time::startStopwatch();

	}
	swt = Time::endStopwatch();
	if (swt > 0.01) {
		cout << "Pollevents time: " << swt << endl;
		cout << "Event count :" << eventCount << endl;
	}

	if (input.GetButtonDown("A")) {
		mainCamera.ProcessKeyboard(LEFT, Time::deltaTime);
	}if (input.GetButtonDown("S")) {
		mainCamera.ProcessKeyboard(BACKWARD, Time::deltaTime);
	}if (input.GetButtonDown("D")) {
		mainCamera.ProcessKeyboard(RIGHT, Time::deltaTime);
	}if (input.GetButtonDown("W")) {
		mainCamera.ProcessKeyboard(FORWARD, Time::deltaTime);
	}

	//Cycle mouse mode vs keyboard only mode. Allows the mouse to be freed
	if (input.OnPress("Escape")) {
		if (mouseMode) {
			mouseMode = false;
			SDL_ShowCursor(1);
		}
		else {
			mouseMode = true;
			SDL_ShowCursor(0);
			SDL_WarpMouseInWindow(renderer.window, 50, 50); //for some reason this doesn't work
		}
	}
	//Mouse input
	if (mouseMode) {
		int xoff;
		int yoff;
		SDL_GetMouseState(&xoff, &yoff);
		mainCamera.ProcessMouseMovement(xoff - 50, -1 * yoff + 50);
		SDL_WarpMouseInWindow(renderer.window, 50, 50);
	}
	
	
	renderer.models[0]->position = vec3(0.0f, 0.0f, 0.0f);
	renderer.models[0]->scale = vec3(0.3f, 0.3f, 0.3f);
	renderer.models[0]->view = mainCamera.GetViewMatrix();//Should have this somewhere else. Likely in Model's per frame call

	renderer.drawFrame();
	
}//close for while(stillRunning)
}//close for mainLoop()