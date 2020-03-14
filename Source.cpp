//Adapted from Vulkan-Tutorial
//Henry Arjet, 2020

#include <arjet/renderer.h>
#include <arjet/camera.h>
#include <arjet/input.h>
#include <arjet/model.h>
#include <arjet/mesh.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
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
		meshes[0].ubo.view = mainCamera.GetViewMatrix();
		meshes[0].position = vec3(0, -1*glm::sin(now), -2.0f);
		meshes[1].ubo.view = mainCamera.GetViewMatrix();
		meshes[1].position = vec3(glm::cos(now), glm::sin(now), 0);
		models[0].meshes[0].ubo.view = mainCamera.GetViewMatrix();
		models[0].meshes[0].position = vec3(0, 0, 0);



		renderer.startFrame();
		meshes[0].updateUniformBuffer(renderer.currentFrame);
		meshes[1].updateUniformBuffer(renderer.currentFrame);
		models[0].meshes[0].updateUniformBuffer(renderer.currentFrame);
		renderer.finishFrame();
	}
}
int main() {
	Renderer renderer;
	//renderer.texturePaths.push_back("chalet.jpg");
	//renderer.texturePaths.push_back("sample_texture.jpg");
	renderer.initVulkan();

	//texture stuff. Temporary
	int texturePathsSize = 2;
	renderer.texturePaths = { "chalet.jpg", "sample_texture.jpg" };
	renderer.textureImages.resize(texturePathsSize);
	renderer.textureImageMemory.resize(texturePathsSize);
	renderer.textureImageViews.resize(texturePathsSize);// I'll have to get arround this resizing at some point. Use mesh.push logic
	renderer.createTextureImage(0, "chalet.jpg");
	renderer.createTextureImage(1, "sample_texture.jpg");

	textureCounter = 2;
	meshCounter = 2;


	meshes.push_back(Mesh(renderer));
	meshes.push_back(Mesh(renderer));


	meshes[0].textures.resize(1);
	meshes[1].textures.resize(1);
	meshes[1].textures[0].texIndex = 1;

	meshes[1].index = 1;
	meshes[1].texIndex = 1;



	models.push_back(Model(renderer, "models/nanosuit/scene.fbx", meshCounter, textureCounter));

	meshes[0].init();
	meshes[1].init();
	models[0].meshes[0].init();
	renderer.finalizeVulkan();

	mainCamera = Camera();
	startTime = SDL_GetPerformanceCounter();
	SDL_ShowCursor(0);
	loop(renderer);
	//renderer.cleanup();
	return 0;
}
