#pragma once
#include <arjet/script.h> //also includes component

class Script;
struct GameObject {
	Script* script = nullptr;
	void update() {
		script->update();
	}
	GameObject() {}//empty constructor
};
