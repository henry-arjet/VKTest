#pragma once
#include <iostream>
#include <arjet/component.h>

class Script : Component {
public:
	void update() {
		std::cout << "Scripting!" << std::endl;
	}
	Script(GameObject & gameObject) : Component(gameObject){}
};