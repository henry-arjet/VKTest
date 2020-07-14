#include <arjet/input.h>
std::map <std::string, int> Input::keys;
std::map <std::string, float> Input::axes;

void Input::Init() {
	keys["A"] = 0;
	keys["S"] = 0;
	keys["D"] = 0;
	keys["W"] = 0;
	keys["Escape"] = 0;
	keys["Q"] = 0;
	keys["E"] = 0;
	keys["R"] = 0;
	keys["1"] = 0;
	keys["2"] = 0;
	keys["3"] = 0;
	keys["4"] = 0;
	keys["Left"] = 0;
	keys["Right"] = 0;
	keys["Up"] = 0;
	keys["Down"] = 0;
}

void Input::ProcessKey(SDL_KeyboardEvent* key) {
	if (key->type == SDL_KEYDOWN) {
		std::string k = SDL_GetKeyName(key->keysym.sym);
		keys[k] = 2;
	}
	else if (key->type == SDL_KEYUP)keys[SDL_GetKeyName(key->keysym.sym)] = 0;
}

bool Input::OnPress(cstr k) { //returns true only once per press
	if (keys[k] == 2) {
		keys[k] = 1; //sets it to not return true again until reset
		return true;
	}
	else return false;
}

bool Input::GetButtonDown(cstr k) {
	return (keys[k] != 0); //accepts either 2 (just pressed) or 1 (held down)
}