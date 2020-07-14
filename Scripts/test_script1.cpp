#include <test_script1.h>
#include <iostream>
#include <arjet/GameObject.h>
#include <arjet/input.h>
void test_script1::start() {
	std::cout << "Test script one started!" << std::endl;
}

void test_script1::update() {
	if (Input::GetButtonDown("Up")){
		gameObject->transform.translate(vec3(0.0f, 0.0f, 1.0f * Time::deltaTime));
	}
	if (Input::GetButtonDown("Down")) {
		gameObject->transform.translate(vec3(0.0f, 0.0f, -1.0f * Time::deltaTime));
	}
	if (Input::GetButtonDown("Left")) {
		gameObject->transform.translate(vec3(1.0f * Time::deltaTime, 0.0f, 0.0f));
	}
	if (Input::GetButtonDown("Right")) {
		gameObject->transform.translate(vec3(-1.0f * Time::deltaTime, 0.0f, 0.0f));
	}
}