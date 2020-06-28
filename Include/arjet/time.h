#pragma once

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

double Time::deltaTime;
double Time::now;
ulong Time::lastTime;
ulong Time::thisTime;
ulong Time::startTime;
ulong Time::stopwatchStart;