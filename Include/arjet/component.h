#pragma once
class GameObject;
class Component {
public:
	Component(GameObject& gameObject) : gameObject(gameObject) {}
	GameObject& gameObject;
};
#include <arjet/game_object.h>
