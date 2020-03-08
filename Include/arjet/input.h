#pragma once
//Fully original

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <map>
#include <string>
#include <iostream>
#define cstr const char*

class Input {
public:
	std::map <std::string, int> keys;
	std::map <std::string, float> axes;

	Input() {
		keys["A"] = 0;
		keys["S"] = 0;
		keys["D"] = 0;
		keys["W"] = 0;
		keys["Escape"] = 0;
	}




	void ProcessKey(SDL_KeyboardEvent *key) {
		if (key->type == SDL_KEYDOWN) {
			std::string k = SDL_GetKeyName(key->keysym.sym);
			keys[k] = 1;
		}
		else if (key->type == SDL_KEYUP)keys[SDL_GetKeyName(key->keysym.sym)] = 0;

	}


	bool GetButtonDown(cstr k) {		
		return keys[k];
	}
};