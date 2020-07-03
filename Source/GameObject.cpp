//I need this source file for literally one damn function
#include <arjet/GameObject.h>

void GameObject::start() {

	for (int i = 0; i < components.size(); i++) { //Call start for every component
		components[i]->start();
	}
}