//I need this source file for literally one damn function
#include <arjet/GameObject.h>

void GameObject::start() {
	for (Component* c : components) { //Call start for every component
		c->start();
	}
}