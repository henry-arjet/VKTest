#pragma once
class GameObject;


class Component{
public:
	GameObject* gameObject;//The parent
	virtual void start() {}//called when the gameObject is first initialized
	virtual void update() {}//called every frame
};