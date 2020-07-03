#pragma once

#include <arjet/Component.h>
#include <arjet/Transform.h>
#include <arjet/Universal.h>
#include <iostream>
#include <vector>


#define cstr const char*
#define assres assert(res == VK_SUCCESS)
#define ushort uint16_t
#define uint uint32_t
#define ulong uint64_t
#define scuint static_cast<uint32_t>

using std::vector;
using std::cout;
using std::endl;

class Transform;

class GameObject{
public:
	vector<std::unique_ptr<Component>> components;
	Transform transform; //Every gameObject will have a transform
	string name;
	GameObject(string name) {	
		this->name = name;
	}
	GameObject(){//depreciated
	}

	void start();

	
};

