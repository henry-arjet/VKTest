#pragma once

#include <arjet/vertex.h>
#include <arjet/renderer.h>
#include <arjet/mesh.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>



class Light { //I really need to create a game object for this to inharit from. I think I'll add that after normal mapping
public: 
	LightInfo info;
	Mesh &mesh;
	Light(Mesh &m) : mesh(m) {	}//Constructor, takes a reference to a mesh. Might clean this up at some point, have them both children of gameobject.
	void updatePositions(vec3 newPos) {//This function updates the positions of both the mesh and the light.
		info.position = newPos;
		mesh.position = newPos;
	}
};