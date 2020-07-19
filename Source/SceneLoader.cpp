#include <arjet/SceneLoader.h>
#include <arjet/Universal.h>
#include <arjet/Component.h>
#include <arjet/Light.h>

#include <test_script1.h>
#include <camera_script.h>
//You may notice I use multiple different patterns for similar opporations. 
//It's because I'm still getting a feel for them, and I want to mess around with multiple approaches

void SceneLoader::load() {
	words = intakeLines(); //This method processes the blocks. So shader block, object block, whatever else I add
	for (ulong i = 0; i < words.size(); i++) {
		switch (sectionFlagLookup(words[i][0])) { //Fun fact switches don't seem to work with strings. Thus, sectionFlagLookup to turn string into enum using if statements, thus wasting all the gains of using a switch. Ugh.
		case SHADERS:
			cout << "Hit shader block" << endl;
			i = processShaders(i+1) - 1;
			break;
		case OBJECTS:
			cout << "Hit objects block" << endl;
			i = processObjects(i + 1) - 1;
			break;
		case END:
			return;
		default:
			cout << "expected section flag, was dissapointed" << endl;
			break;
		}
	}
}

ulong SceneLoader::processObjects(ulong i) { //This method works inside the object block, changes the context until a new block is hit
	while (sectionFlagLookup(words[i][0]) == NONE) { //While we haven't hit a new block
		//This pattern isn't as efficiant as a switch(I think) but it's a lot easier to write
		if (words[i][0] == "Object") { //Creates a new object
			GameObjectPtr tempObject(new GameObject(words[i][1]));
			Universal::gameObjects.push_back(std::move(tempObject)); //constructs a game object, assigns a name from the file
		}
		else if (words[i][0] == "Transform") { //processes a transform
			int j = 1; //The word we're searching for
			while (j < words[i].size()) {
				if (words[i][j] == "Position") {
					Universal::gameObjects.back()->transform.position = vec3(stof(words[i][j + 1]), stof(words[i][j + 2]), stof(words[i][j + 3]));
					j += 4;// Skips the three coordinates
				}
				else if (words[i][j] == "Scale") {
					Universal::gameObjects.back()->transform.size = vec3(stof(words[i][j + 1]), stof(words[i][j + 2]), stof(words[i][j + 3]));
					j += 4;// Skips the three coordinates
				}
				else if (words[i][j] == "Rotation") {
					Universal::gameObjects.back()->transform.rotation = vec3(stof(words[i][j + 1]), stof(words[i][j + 2]), stof(words[i][j + 3]));
					j += 4;// Skips the three coordinates
				}
				else { cout << "Something's wrong with your transform input" << endl; j++; }
			}
		}
		else if (words[i][0] == "Model") {
			std::unique_ptr<Model> tempModel( new Model(Universal::gameObjects.back().get(), Universal::renderer, words[i][1], tcount)); //I don't know smart pointers. I'm just glad this works
			tempModel->type = "Model";
			Universal::gameObjects.back()->components.push_back(std::move(tempModel));
		}
		else if (words[i][0] == "Script") {
			auto ptr = generateScript(words[i][1]);
			ptr->type = "Script";
			Universal::gameObjects.back()->components.push_back(std::move(ptr));
		}
		else if (words[i][0] == "Light") {
			std::unique_ptr<Light> tempLight(new Light());
			tempLight->type = "Light";
			tempLight->active = true;
			tempLight->info.inUse = true;
			tempLight->gameObject = Universal::gameObjects.back().get(); //Store the raw pointer, as the gameObject will delete the light when it itself is deleted
			int j = 1; //The word we're searching for
			while (j < words[i].size()) { //Same pattern as for transform
				if (words[i][j] == "Strength") {
					tempLight->info.strength = stof(words[i][j + 1]);
					j += 2;
				}
			}
			Universal::gameObjects.back()->components.push_back(std::move(tempLight));
		}
		else if (words[i][0] == "Camera"){
			std::unique_ptr<Camera> tempCamera(new Camera());
			tempCamera->type = "Camera";//might want to move type string assignment to the constructor
			tempCamera->gameObject = Universal::gameObjects.back().get();
			Universal::mainCamera = tempCamera.get(); //Just as with light, we don't want to give the Universal class ownership
			Universal::gameObjects.back()->components.push_back(std::move(tempCamera));
		}
		i++;
	}
	return i;
}

std::unique_ptr<Component> SceneLoader::generateScript(string str) { //Takes a string, returns the unique_ptr to the referenced script
	//I'm gonna have to do this manually for each script I create until I make a whole ass script system
	//Right now, I just have a half-ass script system
	if (str == "test_script1") {
		auto ptr = std::unique_ptr<Component>(new test_script1());
		ptr->gameObject = Universal::gameObjects.back().get();
		return ptr;
	}
	else if (str == "camera_script") {
		auto ptr = std::unique_ptr<Component>(new camera_script());
		ptr->gameObject = Universal::gameObjects.back().get();
		return ptr;
	}
	else cout << "Couldn't find the script" << endl;
}


ulong SceneLoader::processShaders(ulong i) {
	vector<ShaderPath> shaderPaths;
	for (i; i < words.size(); i++) {
		if (sectionFlagLookup(words[i][0]) != NONE) { //If we're done with the shaders i.e. found another block
			Universal::renderer.init(shaderPaths);
			return i;
		}
		shaderPaths.push_back(ShaderPath(words[i][0], words[i][1])); //assumes standard 2 shader pipeline layout
	}
	cout << "Something went wrong in processShaders()" << endl; //Should hit another section flag. If it doesn't, something's wrong
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