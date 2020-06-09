//Adapted from Vulkan-Tutorial
//Henry Arjet, 2020

#include <arjet/renderController.h>
#include <arjet/renderer.h>
#include <arjet/camera.h>
#include <arjet/shader.h>
#include <arjet/input.h>
#include <arjet/light.h>
#include <arjet/model.h>
#include <arjet/mesh.h>
#include <arjet/game_object.h>

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
GameObject testObject;


void loop(RenderController &renderController);

int main() {
	Script* testScript = new Script(testObject);
	testObject.script = testScript; //The value testScript should now be useless

	RenderController renderController;
	renderController.init();

	//Texture stuff. Temporary. I could burn this whole thing down, but I'll keep it just for debugging.
	int texturePathsSize = 1;
	renderController.renderer.texturePaths = {"sample_texture.jpg" };
	renderController.renderer.textureImages.resize(texturePathsSize);
	renderController.renderer.textureImageMemory.resize(texturePathsSize);
	renderController.renderer.textureImageViews.resize(texturePathsSize);
	renderController.renderer.createTextureImage(0, "sample_texture.jpg");

	textureCounter = 1;
	meshCounter = 2;


	meshes.push_back(Mesh(renderController.renderer));//mesh 0, the light
	meshes.push_back(Mesh(renderController.renderer));
	lights.push_back(Light(meshes[0])); //creates a light, assigns it mesh 0
	lights[0].info.inUse = true; //Says to the frag shader that we're actually using this light
	lights[0].info.strength = 1.0f;
	meshes[0].textures.resize(1);//initializes with default texture and indexes, all 0
	meshes[1].textures.resize(1);
	meshes[1].index = 1;
	meshes[0].shaderIndex = 1; //Use light source shader (hardcoded white)
	//meshes[0].shaderIndex = 1; //Use light source shader (hardcoded white)

	models.push_back(Model(renderController.renderer, "models/nanosuit/scene.fbx", meshCounter, textureCounter));

	meshes[0].init();
	meshes[1].init();
	for (int j = 0; j < models[0].meshes.size(); j++) { //code to scale, position, and initialize the nanosuit
		models[0].meshes[j].scale = vec3(0.1f, 0.1f, 0.1f);
		models[0].meshes[j].init();
		models[0].meshes[j].position = vec3(0.0f, 0.0f, -0.5f);
	}


	for (int i = 0; i < meshes.size(); i++) {
		//cout << meshes[i].textures.size() << endl;
		//mesh.texIndex = mesh.descriptorSets.size(); // FIXME
		renderController.meshes.push_back(&(meshes[i]));
	}
	for (int i = 0; i < models.size(); i++) {
		for (int j = 0; j < models[i].meshes.size(); j++) {
			renderController.meshes.push_back(&(models[i].meshes[j]));
		}
	}



	renderController.finalizeVulkan();

	mainCamera = Camera();
	startTime = SDL_GetPerformanceCounter();
	SDL_ShowCursor(0);
	loop(renderController);
	//renderer.cleanup();
	return 0;
}


void loop(RenderController &renderController) {
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
					renderController.renderer.framebufferResized = true;
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
				SDL_WarpMouseInWindow(renderController.renderer.window, 50, 50); //for some reason this doesn't work

			}
		}
		if (mouseMode) {
			int xoff;
			int yoff;
			SDL_GetMouseState(&xoff, &yoff);
			mainCamera.ProcessMouseMovement(xoff - 50, -1 * yoff + 50);
			SDL_WarpMouseInWindow(renderController.renderer.window, 50, 50);
		}
		//End of input
		testObject.update();

		//Now lets do actual rendering stuff
		meshes[0].ubo.view = mainCamera.GetViewMatrix(); //still light mesh
		lights[0].updatePositions(vec3(0, -1 * glm::sin(now) + 0.6f, glm::cos(now) - 0.3f));
		meshes[1].ubo.view = mainCamera.GetViewMatrix();
		meshes[1].position = vec3(glm::cos(now), glm::sin(now), 0);
		for (int j = 0; j < models[0].meshes.size(); j++) {
			models[0].meshes[j].ubo.view = mainCamera.GetViewMatrix();
		}
		meshes[1].ubo.lights[0] = lights[0].info;

		for (int j = 0; j < models[0].meshes.size(); j++) {
			models[0].meshes[j].ubo.lights[0] = lights[0].info;
		}

		renderController.renderer.startFrame();
		meshes[0].updateUniformBuffer(renderController.renderer.currentFrame);
		meshes[1].updateUniformBuffer(renderController.renderer.currentFrame);
		for (int j = 0; j < models[0].meshes.size(); j++) {
			models[0].meshes[j].updateUniformBuffer(renderController.renderer.currentFrame);
		}
		renderController.renderer.finishFrame();
	}
}