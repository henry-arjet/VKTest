#pragma once
//Fully original
//Kinda lame. Should redo it

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <map>
#include <string>
#include <iostream>
#define cstr const char*

class Input {
public:
	static std::map <std::string, int> keys;
	static std::map <std::string, float> axes;

	static void Init();

	static void ProcessKey(SDL_KeyboardEvent* key);

	static bool OnPress(cstr k);

	static bool GetButtonDown(cstr k);
};