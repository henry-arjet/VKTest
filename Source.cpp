//Base from Vulkan-Tutorial
//Henry Arjet, 2020

#include <arjet/renderer.h>
#include <arjet/camera.h>
#include <arjet/input.h>
#include <arjet/mesh.h>
#include <arjet/model.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <iostream>


#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Timer {
	std::chrono::steady_clock::time_point startingTime;
	std::chrono::duration<double> elapsed;
	void start() {
		startingTime = std::chrono::high_resolution_clock::now();
	}

	void stop() {
		elapsed = std::chrono::high_resolution_clock::now() - startingTime;
	}

	void print() {
		cout << elapsed.count() * 1000;
	}
};


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
vector<Model> models;
uint meshCount = 0;
uint texCount = 0;
Timer perfTimer;
vector<Mesh> jankMeshs;
vector<Vertex> vertices = {
	{vec3(-0.5f, -0.5f, -0.5f), vec3(0.0f,  0.0f, -1.0f),   vec2(0.0f, 0.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},   //pos it ion, no rm al, texture coord, t a n, b i t
	{vec3( 0.5f, -0.5f, -0.5f), vec3(0.0f,  0.0f, -1.0f),   vec2(1.0f, 0.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3( 0.5f,  0.5f, -0.5f), vec3(0.0f,  0.0f, -1.0f),   vec2(1.0f, 1.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3( 0.5f,  0.5f, -0.5f), vec3(0.0f,  0.0f, -1.0f),   vec2(1.0f, 1.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3(-0.5f,  0.5f, -0.5f), vec3(0.0f,  0.0f, -1.0f),   vec2(0.0f, 1.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)}, 
	{vec3(-0.5f, -0.5f, -0.5f), vec3(0.0f,  0.0f, -1.0f),   vec2(0.0f, 0.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	
	{vec3(-0.5f, -0.5f,  0.5f), vec3(0.0f,  0.0f,  1.0f),   vec2(0.0f, 0.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3( 0.5f, -0.5f,  0.5f), vec3(0.0f,  0.0f,  1.0f),   vec2(1.0f, 0.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3( 0.5f,  0.5f,  0.5f), vec3(0.0f,  0.0f,  1.0f),   vec2(1.0f, 1.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3( 0.5f,  0.5f,  0.5f), vec3(0.0f,  0.0f,  1.0f),   vec2(1.0f, 1.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3(-0.5f,  0.5f,  0.5f), vec3(0.0f,  0.0f,  1.0f),   vec2(0.0f, 1.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3(-0.5f, -0.5f,  0.5f), vec3(0.0f,  0.0f,  1.0f),   vec2(0.0f, 0.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},

	{vec3(-0.5f,  0.5f,  0.5f), vec3(-1.0f,  0.0f, 0.0f),   vec2(0.0f, 0.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3(-0.5f,  0.5f, -0.5f), vec3(-1.0f,  0.0f, 0.0f),   vec2(1.0f, 0.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3(-0.5f, -0.5f, -0.5f), vec3(-1.0f,  0.0f, 0.0f),   vec2(1.0f, 1.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3(-0.5f, -0.5f, -0.5f), vec3(-1.0f,  0.0f, 0.0f),   vec2(1.0f, 1.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3(-0.5f, -0.5f,  0.5f), vec3(-1.0f,  0.0f, 0.0f),   vec2(0.0f, 1.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3(-0.5f,  0.5f,  0.5f), vec3(-1.0f,  0.0f, 0.0f),   vec2(0.0f, 0.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},

	{vec3( 0.5f,  0.5f,  0.5f), vec3( 1.0f,  0.0f, 0.0f),   vec2(0.0f, 0.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3( 0.5f,  0.5f, -0.5f), vec3( 1.0f,  0.0f, 0.0f),   vec2(1.0f, 0.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3( 0.5f, -0.5f, -0.5f), vec3( 1.0f,  0.0f, 0.0f),   vec2(1.0f, 1.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3( 0.5f, -0.5f, -0.5f), vec3( 1.0f,  0.0f, 0.0f),   vec2(1.0f, 1.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3( 0.5f, -0.5f,  0.5f), vec3( 1.0f,  0.0f, 0.0f),   vec2(0.0f, 1.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3( 0.5f,  0.5f,  0.5f), vec3( 1.0f,  0.0f, 0.0f),   vec2(0.0f, 0.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},

	{vec3(-0.5f, -0.5f, -0.5f), vec3(0.0f,  -1.0f, 0.0f),   vec2(0.0f, 0.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3( 0.5f, -0.5f, -0.5f), vec3(0.0f,  -1.0f, 0.0f),   vec2(1.0f, 0.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3( 0.5f, -0.5f,  0.5f), vec3(0.0f,  -1.0f, 0.0f),   vec2(1.0f, 1.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3( 0.5f, -0.5f,  0.5f), vec3(0.0f,  -1.0f, 0.0f),   vec2(1.0f, 1.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3(-0.5f, -0.5f,  0.5f), vec3(0.0f,  -1.0f, 0.0f),   vec2(0.0f, 1.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3(-0.5f, -0.5f, -0.5f), vec3(0.0f,  -1.0f, 0.0f),   vec2(0.0f, 0.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},

	{vec3(-0.5f,  0.5f, -0.5f), vec3(0.0f,   1.0f, 0.0f),   vec2(0.0f, 0.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3( 0.5f,  0.5f, -0.5f), vec3(0.0f,   1.0f, 0.0f),   vec2(1.0f, 0.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3( 0.5f,  0.5f,  0.5f), vec3(0.0f,   1.0f, 0.0f),   vec2(1.0f, 1.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3( 0.5f,  0.5f,  0.5f), vec3(0.0f,   1.0f, 0.0f),   vec2(1.0f, 1.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3(-0.5f,  0.5f,  0.5f), vec3(0.0f,   1.0f, 0.0f),   vec2(0.0f, 1.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3(-0.5f,  0.5f, -0.5f), vec3(0.0f,   1.0f, 0.0f),   vec2(0.0f, 0.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
};

vector<Vertex> vertices2 = {
	{vec3(-0.5f, -0.5f, -0.5f), vec3(0.0f,  0.0f, -1.0f),   vec2(0.0f, 0.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},   //pos it ion, no rm al, texture coord, t a n, b i t
	{vec3( 0.5f, -0.5f, -0.5f), vec3(0.0f,  0.0f, -1.0f),   vec2(1.0f, 0.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
	{vec3( 0.5f,  0.5f, -0.5f), vec3(0.0f,  0.0f, -1.0f),   vec2(1.0f, 1.0f),  vec3(0.0f,0.0f,0.0f), vec3(0.0f,0.0f,0.0f)},
};

vector<uint> indices = {
	0, 1, 2,
	3, 4, 5,

	6, 7, 8,
	9, 10, 11,

	12, 13, 14,
	15, 16, 17,

	18, 19, 20,
	21, 22, 23,

	24, 25, 26,
	27, 28, 29,

	30, 31, 32,
	33, 34, 35 
};

void loop(Renderer &renderer){
	bool stillRunning = true;
	while (stillRunning) {
		perfTimer.start();
		//first calculate delta time
		lastTime = thisTime;
		thisTime = SDL_GetPerformanceCounter();
		deltaTime = (double)((thisTime - lastTime) / (double)SDL_GetPerformanceFrequency());
		now = (double)((thisTime - startTime) / (double)SDL_GetPerformanceFrequency());
		perfTimer.stop();
		//cout << "timing stuff took "; perfTimer.print(); cout << "ms" << endl;
		perfTimer.start();
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
		perfTimer.stop();
		//cout << "input took "; perfTimer.print(); cout << "ms" << endl;

		perfTimer.start();
		for (int i = 0; i < models.size(); ++i) {
			for (int j = 0; j < models[i].meshes.size(); j++) {
				models[i].meshes[j].ubo.view = mainCamera.GetViewMatrix();
			}
		}perfTimer.stop();
		//cout << "pushing view matrix took "; perfTimer.print(); cout << "ms" << endl;

		jankMeshs[0].ubo.view = mainCamera.GetViewMatrix();
		jankMeshs[0].position = vec3(0, 0, -2.0f);
		perfTimer.start();
		//models[0].meshes[0].position = vec3(0, -1*glm::sin(now), -2.0f); // should be controlled by GameObject class
		perfTimer.stop();
		//cout << "pushing position took "; perfTimer.print(); cout << "ms" << endl;
		
		perfTimer.start();
		renderer.startFrame();
		for (int i = 0; i < models.size(); ++i) {
			for (int j = 0; j < models[i].meshes.size(); j++) {
				models[i].meshes[j].updateUniformBuffer(renderer.currentFrame);

			}
		}
		jankMeshs[0].updateUniformBuffer(renderer.currentFrame);
		renderer.finishFrame();
		perfTimer.stop();
		//cout << "rendering took "; perfTimer.print(); cout << "ms" << endl;
	}
}
int main() {
	Renderer renderer;
	renderer.initVulkan();
	perfTimer.start();
	//models.push_back(Model("C:/Users/Henry/source/repos/VKTest/chalet.obj", renderer, meshCount, texCount));
	perfTimer.stop();
	cout << "loading models took "; perfTimer.print(); cout << "ms" << endl;



	Texture hackTex;
	hackTex.index = 0;
	vector <Texture> hackTexes = { hackTex };

	renderer.createTextureImage(0, "textures/chalet.jpg");
	renderer.createTextureImageView(0);


	jankMeshs.push_back(Mesh(renderer, vertices, indices, hackTexes, 0));

	//models[0].meshes[0].vertices = vertices;
	//models[0].meshes[0].indices = indices;
	//models[0].meshes[0].init();
	jankMeshs[0].init();

	renderer.finalizeVulkan();

	mainCamera = Camera();
	startTime = SDL_GetPerformanceCounter();

	//jankMeshs[0] = Mesh(renderer, vertices, indices, hackTexes, 0);
	SDL_ShowCursor(0);
	loop(renderer);
	//renderer.cleanup();
	return 0;
}
