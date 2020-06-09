#pragma once
#include <iostream>
#include <arjet/component.h>

class Script : Component {
public:
	void update() {
	}
	Script(GameObject & gameObject) : Component(gameObject){}
};