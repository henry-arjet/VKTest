#include<arjet/Universal.h>

#define _ITERATOR_DEBUG_LEVEL 0
//For some reason, it crashes on exit if I don't have this

mat4 Universal::viewMatrix; //reference to the main camera's view matrix
Renderer Universal::renderer;
vector<GameObject> Universal::gameObjects;
Input Universal::input;
bool Universal::mouseMode = true;
Camera Universal::mainCamera;

int Universal::run() {
	vector<ShaderPath> shaderPaths;
	shaderPaths.push_back(ShaderPath("Shaders/vert.spv", "Shaders/frag.spv"));
	shaderPaths.push_back(ShaderPath("Shaders/lightV.spv", "Shaders/lightF.spv"));

	renderer.init(shaderPaths);
	
	gameObjects.push_back(GameObject());

	mainCamera = Camera();
	//Universal.viewMatrix must be a thing before I create models
	Universal::viewMatrix = mainCamera.GetViewMatrix();

	uint tCount = 0; //keeps track of textures
	gameObjects[0].components.push_back(new Model(gameObjects[0], renderer, "models/nanosuit/scene.fbx", tCount));

	gameObjects[0].transform->scale(0.3f);


	Time::start();
	mainLoop();
	//Should clean up the rendererer here
	return 0;
}
void Universal::mainLoop() {
bool stillRunning = true;
while (stillRunning) {
	//set deltaTime and 'now' for this frame
	Time::resetDelta();

	//SDL Input
	SDL_Event event;
	uint eventCount = 0; //FOR DEBUG
	while (SDL_PollEvent(&event)) {

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

	//Get the new camera position to Universal
	viewMatrix = mainCamera.GetViewMatrix();

	renderer.drawFrame();

}//close while(stillRunning)
}//close mainLoop()

Universal::~Universal() {
	cout << "TEST" << endl;
}