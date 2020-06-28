#pragma once
#include<arjet/mesh.h>
#include<assimp/Importer.hpp>
#include<assimp/scene.h>
#include<assimp/postprocess.h>


class Mesh;
struct Texture;
class Renderer;

void TextureFromFile(Renderer& renderer, uint count, const char* path, const string& directory);


class Model {
public:
	bool draw = true;
	vec3 position;
	vec3 scale;
	mat4& view = mat4(1); //Point each model to a universal view vec3
	vector<VkCommandBuffer> buffers;
	//functions
	Model(Renderer& r, string const& path, /*uint& mCountmesh counter. How many meshes have been processed. Don't think I need this,*/
	  uint& tCount/*texture counter*/) : renderer(r), textureCounter(tCount) {
		loadModel(path);
		createSecondaryBuffers();
	}
	vector<Mesh*> meshes;

private:
	string directory;
	vector<Texture> textures_loaded;
	Renderer& renderer;
	//uint& meshCounter;    //used for assigning index numbers to
	uint& textureCounter;  //meshes and textures

	
	void loadModel(string const& path); //Opens the scene and calls processNode() for the root node
	void processNode(aiNode* node, const aiScene* scene); //Recursively processes all nodes
	Mesh* processMesh(aiMesh* mesh, const aiScene* scene);
	vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName);
	void createSecondaryBuffers();
};



