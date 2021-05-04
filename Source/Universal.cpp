#include<arjet/Universal.h>

mat4 Universal::viewMatrix; //reference to the main camera's view matrix
Renderer Universal::renderer;
vector<GameObjectPtr> Universal::gameObjects;
bool Universal::mouseMode = true;
Camera* Universal::mainCamera;


int Universal::run() {
	//USE A DOUBLE POINTER. So have a pointer to universal::viewMatrix, and then have that be a pointer to mainCamera::viewMatrix
	//OR I could put that stuff in the start() function. So I load the camera, push Universal::viewMatrix, and then have each model's start() function apply that view matrix
	//Which I think would be faster in execution but a little more complex
	//Universal.viewMatrix must be a thing before I create models
	
	renderer.graphicsOptions.vsync = true;

	SceneLoader loader;
	loader.load();

	Universal::viewMatrix = mat4(0.0f); //Now that we have a camera, we need to add its veiwMatrix before we initialize the models with start()


	Input::Init();
	Time::start();

	for (int i = 0; i < gameObjects.size(); i++) {
		cout << "Starting " << gameObjects[i]->name << endl;
		gameObjects[i]->start();
	}
	Time::resetDelta(); //just so it doesn't count all the loading and starting in the first deltaTime.
	mainLoop();
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
	

	//Get the new camera position to Universal
	viewMatrix = mainCamera->GetViewMatrix();

	//call update for every GameObject
	for (int i = 0; i < gameObjects.size(); i++) {
		gameObjects[i]->update();
	}

	renderer.drawFrame();

}//close while(stillRunning)
}//close mainLoop()