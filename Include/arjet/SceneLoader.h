#pragma once
//loads the scene from a file
//This file format works by flags. There will be a all caps flag that determines the section (eg game objects, shaders)
//Each line will be processed in the manner proscribed to this section until another section flag comes up
//There will also be inline sub flags that specify how the rest of the words in the line are to be applied
#include <iostream>	
#include <fstream>
#include <vector>
#include <string>

using std::vector;
using std::string;


class SceneLoader
{
	std::ifstream sceneFile;
public:
	void load(){
		vector<vector<string>> words = intakeLines();

	}
private:
	vector<vector<string>> intakeLines(){ //Opens the file, reads all the words into a 2d vector split by lines
		sceneFile.open("Scenes/test.txt");
		string line;
		vector<vector<string>> ret;
		while (getline(sceneFile, line)) {
			vector<string> pb = getWords(line);
			ret.push_back(pb);
		}
		sceneFile.close();
		return ret;
	}

	vector<string> getWords(string line, string delimiter = " ") {
		vector<string> ret;
		size_t last = 0;
		size_t next = 0;
		while ((next = line.find(delimiter, last)) != string::npos) {
			ret.push_back(line.substr(last, next - last));
			last = next + 1;
		}
		//return the final word
		ret.push_back(line.substr(last, string::npos));
		return ret;
	}
};

