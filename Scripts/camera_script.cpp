#include "camera_script.h"
#include <arjet/GameObject.h>
#include <arjet/input.h>
int cameraModuleIndex; //Where in the gameObject's vector of modules is the camera module
#define mainCamera (*static_cast<Camera*>(gameObject->components[cameraModuleIndex].get())) //I feel dirty writing this
void camera_script::start() {
	for (int i = 0; i < gameObject->components.size(); i++){//For each of daddy object's components
		if (gameObject->components[i]->type == "Camera") {
			cameraModuleIndex = i;
			break;
		}
	}
}
void camera_script::update() {
	if (Input::GetButtonDown("A")) {
		mainCamera.ProcessKeyboard(LEFT, Time::deltaTime);
	}if (Input::GetButtonDown("S")) {
		mainCamera.ProcessKeyboard(BACKWARD, Time::deltaTime);
	}if (Input::GetButtonDown("D")) {
		mainCamera.ProcessKeyboard(RIGHT, Time::deltaTime);
	}if (Input::GetButtonDown("W")) {
		mainCamera.ProcessKeyboard(FORWARD, Time::deltaTime);
	}
	//Mouse input
	if (Universal::mouseMode) {
		int xoff;
		int yoff;
		SDL_GetMouseState(&xoff, &yoff);
		mainCamera.ProcessMouseMovement(xoff - 50, -1 * yoff + 50);
		SDL_WarpMouseInWindow(Universal::renderer.window, 50, 50);
	}
}