//Adapted from Vulkan-Tutorial
//Henry Arjet, 2020

#include <arjet/renderer.h>
#include <arjet/camera.h>
#include <arjet/input.h>

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

double deltaTime = 0; //delta time in seconds
uint64_t lastTime, now; //helper for deltaTime
Input input;
bool mouseMode = true;


void loop(Renderer &renderer){
	bool stillRunning = true;
	while (stillRunning) {
		//first calculate delta time
		lastTime = now;
		now = SDL_GetPerformanceCounter();
		deltaTime = (double)((now - lastTime) / (double)SDL_GetPerformanceFrequency());
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
		renderer.ubo.view = mainCamera.GetViewMatrix();
		
		renderer.drawFrame();
	}
}
int main() {
	Renderer renderer;
	renderer.texturePaths.push_back("chalet.jpg");
	renderer.texturePaths.push_back("sample_texture.jpg");
	renderer.initVulkan();
	mainCamera = Camera();
	SDL_ShowCursor(0);
	loop(renderer);
	renderer.cleanup();
	return 0;
}
