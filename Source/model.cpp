#include "arjet/model.h"
#include <arjet/Universal.h>
#include <arjet/time.h>

void Model::start() {//Called by parent gameObject
	view = &Universal::viewMatrix; //We need to make sure the camera exists and Universal::viewMatrix exists first;
}

std::vector<Texture> Model::textures_loaded;
std::vector<std::thread> textureThreads;


Model::Model(GameObject* gameObject, Renderer& r, string const& path, uint& tCount) : renderer(r), textureCounter(tCount) {
	this->gameObject = gameObject;
	loadModel(path);
	r.models.push_back(this);
	for (int i = 0; i < textureThreads.size(); i++) { //all textures must be loaded before secondary buffers 
															//or mesh descriptor sets are created
		try {
			textureThreads[i].join();
		}
		catch (std::exception ex) {
			cout << "STD::EXCEPTION IN JOIN: " << ex.what() << endl;
		}
	}
	textureThreads.clear();
	for (int i = 0; i < meshes.size(); i++) {
		meshes[i].createDescriptorSets();
	}
	createSecondaryBuffers();
}
void Model::loadModel(string const& path) {
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		cout << "ERROR::ASSIMP::" << importer.GetErrorString() << endl;
	}
	directory = path.substr(0, path.find_last_of('/'));
	processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene) {
	//process all meshes in node
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(processMesh(mesh, scene));
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		processNode(node->mChildren[i], scene);
	}
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene) {
	//This whole thing works by copying vertices. Could speed it up by passing by reference. But then who would own the original?
	vector<Vertex>vertices;
	//vertices.reserve(mesh->mNumVertices);
	vector<unsigned int> indices;
	vector<Texture> textures;
	uint flags = 0; //Stores the shader feature flags
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
		//cout << "Unknown textures: " << material->GetTextureCount(aiTextureType_UNKNOWN) << endl;
		//cout << "Diff, spec, and norm/height: " << material->GetTextureCount(aiTextureType_DIFFUSE) << " " << material->GetTextureCount(aiTextureType_SPECULAR) << " " << material->GetTextureCount(aiTextureType_HEIGHT) << " " << endl;
		if (material->GetTextureCount(aiTextureType_HEIGHT) > 0) { //If it has normal map, flag it as such for the shader
			flags = flags | ARJET_SHADER_FLAG_NORMAL;
		}
		vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
		vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
		vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");

		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
	}

	return Mesh(renderer, vertices, indices, textures, flags, this); 
}

vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName) {
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
			TextureFromFile(renderer, textureCounter, str.C_Str(), directory); //Adds that texture to the renderer at the index textureCounter
			Texture texture;
			texture.texIndex = textureCounter++; // textureCounter++ so it passes it's value, then itterates. So we don't have the first one as 1, the second as 2, etc.
			//cout << "And storing in index " << texture.texIndex << endl;
			texture.type = typeName; //Not used right now. Should use it.
			texture.path = str.C_Str();
			textures.push_back(texture);
			textures_loaded.push_back(texture);
		}
	}
	return textures;
}

void Model::createSecondaryBuffers() {
	buffers.resize(renderer.swapchainImages.size());
	buffers = renderer.createCommandBuffersModel(&meshes);
}

void Model::recreateSecondaryBuffers() {
	vkFreeCommandBuffers(renderer.device, renderer.commandPool, buffers.size(), buffers.data());
	buffers.resize(renderer.swapchainImages.size());
	buffers = renderer.createCommandBuffersModel(&meshes);
}
void TextureFromFile(Renderer& renderer, uint count, const char* path, const string& directory)
{
	string filename = string(path);
	filename = filename.substr(filename.find_last_of('\\') + 1, filename.size());

	filename = directory + "/textures/" + filename;

	cout << "loading " << filename << endl;
	if (renderer.textureImages.size() <= count) {
		renderer.textureImages.resize(size_t(count) + 1);
		renderer.textureImageViews.resize(size_t(count) + 1);
		renderer.textureImageMemory.resize(size_t(count) + 1);
	}
	textureThreads.push_back(std::thread(&Renderer::createTextureImage, &renderer, count, strdup(filename.c_str())));
}

void Model::clean() {

}