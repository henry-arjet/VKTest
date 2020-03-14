//adapted from learnopengl


#ifndef MODEL_H
#define MODEL_H


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <arjet/mesh.h>
#include <arjet/renderer.h>
#include <arjet/vertex.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>

using std::vector;


uint TextureFromFile(Renderer &renderer, uint count, const char *path, const string &directory, bool gamma = false);

class Model {
public:
	//functions
	Model(Renderer &r, string const &path, uint &mCount, uint &tCount) : renderer(r), meshCounter(mCount), textureCounter(tCount) {
		loadModel(path);
	}
	
	/*void Draw(Shader shader) {

		for (unsigned int i = 0; i < meshes.size(); i++) {
			meshes[i].Draw(shader);
		}

	}*/

	vector<Mesh> meshes; //needs to be accessable
private:
	//model data
	string directory;
	vector<Texture> textures_loaded;
	Renderer& renderer;
	uint& meshCounter;    //used for assigning index numbers to
	uint& textureCounter;  //meshes and textures

	//functions
	void loadModel(string const &path) {
		Assimp::Importer importer;
		const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
		//cout << scene->mNumMaterials << endl;
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			cout << "ERROR::ASSIMP::" << importer.GetErrorString() << endl;
		}
		directory = path.substr(0, path.find_last_of('/'));
		processNode(scene->mRootNode, scene);
	}
	void processNode(aiNode *node, const aiScene *scene) {

		
		//process all meshes in node
		for (unsigned int i = 0; i < node->mNumMeshes; i++) {
			aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
			meshes.push_back(processMesh(mesh, scene));
		}

		for (unsigned int i = 0; i < node->mNumChildren; i++) {
			processNode(node->mChildren[i], scene);
		}
	}
	Mesh processMesh(aiMesh *mesh, const aiScene *scene) {
		vector<Vertex>vertices;
		vector<unsigned int> indices;
		vector<Texture> textures;

		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			Vertex vertex;
			//process positions, normals, and texcoords
			glm::vec3 vector;
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.pos = vector;

			vector.x = mesh->mNormals[i].x;
			vector.y = mesh->mNormals[i].y;
			vector.z = mesh->mNormals[i].z;
			vertex.norm = vector;

			if (mesh->mTextureCoords[0]) { // does it have any?
				glm::vec2 vec;
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.texCoord = vec;
			}
			else vertex.texCoord = glm::vec2(0.0f, 0.0f);

			vector.x = mesh->mTangents[i].x; //We know it has tan and bitangent because I asked assimp to create them
			vector.y = mesh->mTangents[i].y;
			vector.z = mesh->mTangents[i].z;
			vertex.tan = vector;

			vector.x = mesh->mBitangents[i].x;
			vector.y = mesh->mBitangents[i].y;
			vector.z = mesh->mBitangents[i].z;
			vertex.bit = vector;
			vertices.push_back(vertex);
		}

		//process indices
		for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++) {
				indices.push_back(face.mIndices[j]);
			}
		}
		//process material
		if (mesh->mMaterialIndex >= 0) { //again does it have any

			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			cout << "Unknown textures: " << material->GetTextureCount(aiTextureType_UNKNOWN) << endl;
			cout << "Diff, spec, and norm/height: " << material->GetTextureCount(aiTextureType_DIFFUSE) << " " << material->GetTextureCount(aiTextureType_SPECULAR) << " " << material->GetTextureCount(aiTextureType_HEIGHT) << " " << endl;
			vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
			vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
			vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");

			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
			textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
			textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());


		}
		return Mesh(renderer, vertices, indices, textures, meshCounter++);//iterates here
	}
	vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName){
		vector<Texture> textures;
		for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
			aiString str;
			mat->GetTexture(type, i, &str);
			bool skip = false;
			for (unsigned int j = 0; j < textures_loaded.size(); j++) {
				if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
					textures.push_back(textures_loaded[j]);
					skip = true;
					break;
				}
			}

			if (!skip) {
				TextureFromFile(renderer, textureCounter, str.C_Str(), directory);
				Texture texture;
				texture.texIndex = textureCounter++; // tc++ so it passes it's value, then itterates. So we don't have the first one as 1, the second as 2, etc.
				texture.type = typeName;
				texture.path = str.C_Str();
				textures.push_back(texture);
				textures_loaded.push_back(texture);
			}
		}
		return textures;
	}
};

uint TextureFromFile(Renderer &renderer, uint count, const char *path, const string &directory, bool gamma)
{
	string filename = string(path);
	filename = filename.substr(filename.find_last_of('\\') + 1, filename.size());

	filename = directory + "/textures/" + filename;

	cout << "loading " << filename << endl;
	if (renderer.textureImages.size() <= count) {
		renderer.textureImages.resize(count + 1);
		renderer.textureImageViews.resize(count + 1);
		renderer.textureImageMemory.resize(count + 1);
	}

	renderer.createTextureImage(count, filename.c_str());

	return count;
}



#endif


