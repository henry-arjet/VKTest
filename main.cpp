//This entire file exists only because I don't know how to move main()



// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED


#include <arjet/Universal.h>

int main() {
	int i = Universal::run();
	exit(0);
	return 0;
}