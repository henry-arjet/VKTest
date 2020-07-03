#pragma once
//loads the scene from a file
//This file format works by flags. There will be a all caps flag that determines the section (eg game objects, shaders)
//Each line will be processed in the manner proscribed to this section until another section flag comes up
//There will also be inline sub flags that specify how the rest of the words in the line are to be applied
#include <iostream>	
#include <fstream>
#include <vector>
#include <string>

#define cstr const char*
#define assres assert(res == VK_SUCCESS)
#define ushort uint16_t
#define uint uint32_t
#define ulong uint64_t
#define scuint static_cast<uint32_t>

using std::vector;
using std::cout;
using std::endl;
using std::string;


class SceneLoader
{
	std::ifstream sceneFile;
public:
	void load();
private:
	vector<vector<string>> words;

	ulong processShaders(ulong i);

	ulong processObjects(ulong i);

	uint tcount = 0; //Used for loading textures into the renderer

	enum sectionFlag {
		NONE,
		SHADERS,
		OBJECTS,
		END
	};
	sectionFlag sectionFlagLookup(const string& str);


	vector<vector<string>> intakeLines(); //Opens the file, reads all the words into a 2d vector split by lines
		
	vector<string> getWords(string line, string delimiter = ", ");//Gets the words in a line
};

