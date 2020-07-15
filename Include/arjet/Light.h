#pragma once
//Light is a component which can be added to a game object
//It must be manualy given to each mesh
//Right now that is handled by Universal's start function
#include <arjet/Component.h>
#include <arjet/UBO.h>
#include <arjet/GameObject.h>
class Light : public Component{
public:
	bool active = true;
	LightInfo info;

	void update() {
		info.position = gameObject->transform.position;
	}


};

