#pragma once
//These are the children of game object. 
//They include lights, models, transforms, scripts
#include <string>

class GameObject;


class Component{
public:
	GameObject* gameObject;//The parent
	std::string type = "Undefined Component";
	virtual void start() {}//called when the gameObject is first initialized
	virtual void update() {}//called every frame
};

//#include <arjet/GameObject.h>//Have to do it after defining component so transform can inherit from component so GameObject can use transform
//Should probably just switch transform to a unique pointer like all my other components