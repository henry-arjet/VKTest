#pragma once
//Light is a component which can be added to a game object
//It must be manualy given to each mesh
#include <arjet/Component.h>
#include <arjet/UBO.h>
#include <arjet/GameObject.h>
class Light : public Component{
	LightInfo info;
	void update() {
		//update position every frame
		info.position = gameObject->transform.position;
	}
};

