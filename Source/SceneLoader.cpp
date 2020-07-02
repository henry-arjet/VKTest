#include <arjet/SceneLoader.h>
#include <arjet/Universal.h>

void SceneLoader::load() {
	words = intakeLines();
	for (ulong i = 0; i < words.size(); i++) {
		switch (sectionFlagLookup(words[i][0])) {
		case SHADERS:
			cout << "Hit shader block" << endl;
			i = processShaders(i+1) - 1;
			break;
		case OBJECTS:
			cout << "Hit objects block" << endl;
			break;
		default:
			cout << "expected section flag, was dissapointed" << endl;
			break;
		}
	}
}
ulong SceneLoader::processShaders(ulong i) {
	vector<ShaderPath> shaderPaths;
	for (i; i < words.size(); i++) {//will this work?
		if (sectionFlagLookup(words[i][0]) != NONE) {//If we're done with the shaders i.e. found another block
			Universal::renderer.init(shaderPaths);
			return i;
		}
		shaderPaths.push_back(ShaderPath(words[i][0], words[i][1])); //assumes standard 2 shader pipeline layout
	}
	cout << "Something went wrong in processShaders()" << endl;//Should hit another section flag. If it doesn't, something's wrong
	return i;
}

SceneLoader::sectionFlag SceneLoader::sectionFlagLookup(const string& str) {
	if (str == "SHADERS") return SHADERS;
	if (str == "OBJECTS") return OBJECTS;
	if (str == "END") return END;
	return NONE;
}


vector<vector<string>> SceneLoader::intakeLines() { //Opens the file, reads all the words into a 2d vector split by lines
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

vector<string> SceneLoader::getWords(string line, string delimiter) {
	vector<string> ret;
	size_t last = 0;
	size_t next = 0;
	while ((next = line.find(delimiter, last)) != string::npos) {
		ret.push_back(line.substr(last, next - last));
		last = next + delimiter.size();
	}
	//return the final word
	ret.push_back(line.substr(last, string::npos));
	return ret;
}