//Adapted from Vulkan-Tutorial
//Henry Arjet, 2020

#include <arjet/renderer.h>
#include <arjet/camera.h>
#include <arjet/shader.h>
#include <arjet/input.h>
#include <arjet/light.h>
#include <arjet/model.h>
#include <arjet/mesh.h>

#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL.h>
#include <iostream>


#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using glm::mat4;
using glm::mat3;
using glm::vec4;
using glm::vec3;
using glm::vec2;

Camera mainCamera;

double deltaTime, now = 0; //delta times and current time passed in second
uint64_t lastTime, thisTime, startTime; //helper for time keeping
Input input;
bool mouseMode = true;
vector<Mesh> meshes;
vector<Model> models;
uint meshCounter;    //used for assigning index numbers to
uint textureCounter;  //meshes and textures
vector<Light> lights;




void loop(Renderer &renderer){
	bool stillRunning = true;
	while (stillRunning) {
		//first calculate delta time
		lastTime = thisTime;
		thisTime = SDL_GetPerformanceCounter();
		deltaTime = (double)((thisTime - lastTime) / (double)SDL_GetPerformanceFrequency());
		now = (double)((thisTime - startTime) / (double)SDL_GetPerformanceFrequency());

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			//SDL Events
			switch (event.type) {

			case SDL_QUIT:
				stillRunning = false;
				break;
			case SDL_WINDOWEVENT:
				switch (event.window.event) {
				case SDL_WINDOWEVENT_RESIZED:
					renderer.framebufferResized = true;
					break;
				default:
					break;
				}

			case SDL_KEYDOWN: //switches are weird
			case SDL_KEYUP:

				input.ProcessKey(&event.key);
				break;
			default:
				// Do nothing.
				break;
			}
		}
		//Main Logic
		//Input
		if (input.GetButtonDown("A")) {
			mainCamera.ProcessKeyboard(LEFT, deltaTime);
		}if (input.GetButtonDown("S")) {
			mainCamera.ProcessKeyboard(BACKWARD, deltaTime);
		}if (input.GetButtonDown("D")) {
			mainCamera.ProcessKeyboard(RIGHT, deltaTime);
		}if (input.GetButtonDown("W")) {
			mainCamera.ProcessKeyboard(FORWARD, deltaTime);
		}if (input.OnPress("Escape")) {
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
		if (mouseMode) {
			int xoff;
			int yoff;
			SDL_GetMouseState(&xoff, &yoff);
			mainCamera.ProcessMouseMovement(xoff - 50, -1 * yoff + 50);
			SDL_WarpMouseInWindow(renderer.window, 50, 50);
		}
		//End of input
		meshes[0].ubo.view = mainCamera.GetViewMatrix(); //still light mesh
		lights[0].updatePositions (vec3(0, -1*glm::sin(now) + 0.6f, glm::cos(now) - 0.3f));
		meshes[1].ubo.view = mainCamera.GetViewMatrix();
		meshes[1].position = vec3(glm::cos(now), glm::sin(now), 0);
		for (int j = 0; j < models[0].meshes.size(); j++) {
			models[0].meshes[j].ubo.view = mainCamera.GetViewMatrix();
		}

		meshes[1].ubo.lights[0] = lights[0].info;
		for (int j = 0; j < models[0].meshes.size(); j++) {
			models[0].meshes[j].ubo.lights[0] = lights[0].info;
		}
		//cout << meshes[1].ubo.lights[0].strength << endl; 
		renderer.startFrame();
		meshes[0].updateUniformBuffer(renderer.currentFrame);
		meshes[1].updateUniformBuffer(renderer.currentFrame);
		for (int j = 0; j < models[0].meshes.size(); j++) {
			models[0].meshes[j].updateUniformBuffer(renderer.currentFrame);
		}
		renderer.finishFrame();
	}
}
int main() {
	Renderer renderer;
	renderer.initVulkan();
	//Do shaders
	renderer.shaders.push_back(Shader("Shaders/vert.spv", "Shaders/frag.spv", 0, renderer.device));
	renderer.shaders.push_back(Shader("Shaders/lightV.spv", "Shaders/lightF.spv", 1, renderer.device));
	renderer.layThePipe(); //I should change the shader to have an init function called from createPipeline so I don't have to split it up like this
	//On that note I should do the same with meshes

	//Texture stuff. Temporary. I could burn this whole thing down, but I'll keep it just for debugging.
	int texturePathsSize = 1;
	renderer.texturePaths = {"sample_texture.jpg" };
	renderer.textureImages.resize(texturePathsSize);
	renderer.textureImageMemory.resize(texturePathsSize);
	renderer.textureImageViews.resize(texturePathsSize);
	renderer.createTextureImage(0, "sample_texture.jpg");

	textureCounter = 1;
	meshCounter = 2;


	meshes.push_back(Mesh(renderer));//mesh 0
	meshes.push_back(Mesh(renderer));
	lights.push_back(Light(meshes[0])); //creates a light, assigns it mesh 0
	lights[0].info.inUse = true; //Says to the frag shader that we're actually using this light
	lights[0].info.strength = 1.0f;
	meshes[0].textures.resize(1);//initializes with default texture and indexes, all 0
	meshes[1].textures.resize(1);
	meshes[1].index = 1;
	meshes[0].shaderIndex = 1; //Use light source shader (hardcoded white)

	models.push_back(Model(renderer, "models/nanosuit/scene.fbx", meshCounter, textureCounter));

	meshes[0].init();
	meshes[1].init();
	for (int j = 0; j < models[0].meshes.size(); j++) {
		models[0].meshes[j].scale = vec3(0.1f, 0.1f, 0.1f);
		models[0].meshes[j].init();
		models[0].meshes[j].position = vec3(0.0f, 0.0f, -0.5f);
	}

	renderer.finalizeVulkan();

	mainCamera = Camera();
	startTime = SDL_GetPerformanceCounter();
	SDL_ShowCursor(0);
	loop(renderer);
	//renderer.cleanup();
	return 0;
}
