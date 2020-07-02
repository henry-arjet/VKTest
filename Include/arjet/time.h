#pragma once
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL.h>
#include <iostream>
#include <vector>

#define cstr const char*
#define assres assert(res == VK_SUCCESS)
#define ushort uint16_t
#define uint uint32_t
#define ulong uint64_t
#define scuint static_cast<uint32_t>
#define ARJET_SHADER_FLAG_NORMAL 1 // flag for telling my shaders this mesh uses normal maps

using std::vector;
using std::cout;
using std::endl;

class Time {
public:
	static double deltaTime, now;
	static void start() {
		startTime = SDL_GetPerformanceCounter();
	}
	static void resetDelta() {
		lastTime = thisTime;
		thisTime = SDL_GetPerformanceCounter();
		deltaTime = (double)((thisTime - lastTime) / (double)SDL_GetPerformanceFrequency());
		now = (double)((thisTime - startTime) / (double)SDL_GetPerformanceFrequency());
	}
	static void startStopwatch() {
		if (stopwatchStart != 0) {
			cout << "Error cannot start stopwatch; watch already running" << endl;
		}
		else stopwatchStart = SDL_GetPerformanceCounter();
	}
	static double endStopwatch() {
		double ret = (double)((SDL_GetPerformanceCounter() - stopwatchStart) / (double)SDL_GetPerformanceFrequency());
		stopwatchStart = 0;
		return ret;
	}

private:
	static ulong lastTime, thisTime, startTime, stopwatchStart;
};

