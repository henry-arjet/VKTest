//I need this source file for literally one damn function
#include <arjet/GameObject.h>
#include <arjet/Component.h>

void GameObject::start() {

	for (int i = 0; i < components.size(); i++) { //Call start for every component
		components[i]->start();
	}
}

void GameObject::update() {

	for (int i = 0; i < components.size(); i++) { //Call update for every component
		components[i]->update();
	}
}