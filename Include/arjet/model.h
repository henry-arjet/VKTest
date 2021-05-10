#pragma once
#include<arjet/mesh.h>
#include<arjet/Component.h>

#include<assimp/Importer.hpp>
#include<assimp/scene.h>
#include<assimp/postprocess.h>


class Mesh;
struct Texture;
class Renderer;

void TextureFromFile(Renderer& renderer, uint count, const char* path, const string& directory);


class Model : public Component {
public:
	void start();
	bool draw = true;
	//vec3 position; Moved to transform
	//vec3 scale;
	mat4* view; //Point each model to a universal view vec3
	vector<VkCommandBuffer> buffers;
	vector<Mesh> meshes;
	//functions
	Model(GameObject* gameObject, Renderer& r, string const& path, uint& tCount);
	void clean(); //frees up all the renderer handles
	void recreateSecondaryBuffers();
private:
	string directory;
	static vector<Texture> textures_loaded;
	Renderer& renderer;
	//uint& meshCounter;    //used for assigning index numbers to
	uint& textureCounter;  //meshes and textures

	
	void loadModel(string const& path); //Opens the scene and calls processNode() for the root node
	void processNode(aiNode* node, const aiScene* scene); //Recursively processes all nodes
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName); //depricated
	Texture loadMaterialTexture(aiMaterial* mat, aiTextureType type, string typeName);
	void createSecondaryBuffers();
};



