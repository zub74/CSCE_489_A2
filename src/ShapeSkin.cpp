#include <iostream>
#include <fstream>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "ShapeSkin.h"
#include "GLSL.h"
#include "Program.h"
#include "TextureMatrix.h"

using namespace std;
using namespace glm;

ShapeSkin::ShapeSkin() :
	prog(NULL),
	elemBufID(0),
	posBufID(0),
	norBufID(0),
	texBufID(0)
{
	T = make_shared<TextureMatrix>();
}

ShapeSkin::~ShapeSkin()
{
	free(bindPose);
	free(invertedBindPose);
	
	for (int i = 0; i < sz; i++) {
		free(animationFrames[i]);
	}
	free(animationFrames);

	for (int i = 0; i < sz; i++) {
		free(invertedAnimationFrames[i]);
	}
	free(invertedAnimationFrames);
}

void ShapeSkin::setTextureMatrixType(const std::string &meshName)
{
	T->setType(meshName);
}

void ShapeSkin::loadMesh(const string &meshName)
{
	// Load geometry
	// This works only if the OBJ file has the same indices for v/n/t.
	// In other words, the 'f' lines must look like:
	// f 70/70/70 41/41/41 67/67/67
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	string warnStr, errStr;
	bool rc = tinyobj::LoadObj(&attrib, &shapes, &materials, &warnStr, &errStr, meshName.c_str());
	if(!rc) {
		cerr << errStr << endl;
	} else {
		posBuf = attrib.vertices;
		norBuf = attrib.normals;
		texBuf = attrib.texcoords;
		assert(posBuf.size() == norBuf.size());
		// Loop over shapes
		for(size_t s = 0; s < shapes.size(); s++) {
			// Loop over faces (polygons)
			const tinyobj::mesh_t &mesh = shapes[s].mesh;
			size_t index_offset = 0;
			for(size_t f = 0; f < mesh.num_face_vertices.size(); f++) {
				size_t fv = mesh.num_face_vertices[f];
				// Loop over vertices in the face.
				for(size_t v = 0; v < fv; v++) {
					// access to vertex
					tinyobj::index_t idx = mesh.indices[index_offset + v];
					elemBuf.push_back(idx.vertex_index);
				}
				index_offset += fv;
				// per-face material (IGNORE)
				shapes[s].mesh.material_ids[f];
			}
		}
	}

	//apparently you can just reserve size for vectors so theres actually no point in using arrays, but I aready wrote lots of code using arrays
	//so i guess it just be like that sometimes :/
	//animPosBuf.reserve(posBuf.size());
	//animNorBuf.reserve(norBuf.size());
	//jk can't just reserve then access, gotta actually have stuff in there
	animPosBuf = vector<float>(posBuf.size(), 0.0f);
	animNorBuf = vector<float>(norBuf.size(), 0.0f);
}

void ShapeSkin::loadAttachment(const std::string &filename)
{
	ifstream in;
	in.open(filename);
	if (!in.good()) {
		cout << "Cannot read " << filename << endl;
		return;
	}
	cout << "Loading " << filename << endl;

	string line;
	while (true) {
		getline(in, line);
		if (in.eof()) {
			break;
		}
		if (line.empty()) {
			continue;
		}
		// Skip comments
		if (line.at(0) == '#') {
			continue;
		}
		break; //Once we get to actual data break out
	}
	string vertStr;
	string boneStr;
	string maxInfStr;

	stringstream ss(line);

	ss >> vertStr;
	ss >> boneStr;
	ss >> maxInfStr;
	vertices = stoi(vertStr);
	bones = stoi(boneStr);
	maxInfluences = stoi(maxInfStr);
	string str;


	for (int i = 0; i < vertices; i++) {
		getline(in, line);
		stringstream ss(line);
		ss >> str;

		int boneCount = stoi(str);

		if (boneCount > maxInfluences) {
			cout << "too many influences on vertex " << i << "!" << endl;
			return;
		}
		else if (boneCount < 1) {
			cout << "no influences on vertex " << i << "!" << endl;
			return;
		}

		for (int j = 0; j < 9; j++) {
			if (j < boneCount) {
				ss >> str;
				boneIndBuf.push_back(stoi(str));
				ss >> str;
				boneWeightBuf.push_back(stof(str));
			}
			else {
				boneIndBuf.push_back(0);
				boneWeightBuf.push_back(0.0f);
			}
		}
	}
	in.close();
}

void ShapeSkin::init()
{
	// Send the position array to the GPU
	glGenBuffers(1, &posBufID);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_STATIC_DRAW);
	
	// Send the normal array to the GPU
	glGenBuffers(1, &norBufID);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_STATIC_DRAW);
	
	// Send the texcoord array to the GPU
	glGenBuffers(1, &texBufID);
	glBindBuffer(GL_ARRAY_BUFFER, texBufID);
	glBufferData(GL_ARRAY_BUFFER, texBuf.size()*sizeof(float), &texBuf[0], GL_STATIC_DRAW);
	
	// Send the element array to the GPU
	glGenBuffers(1, &elemBufID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBufID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elemBuf.size()*sizeof(unsigned int), &elemBuf[0], GL_STATIC_DRAW);
	
	// Unbind the arrays
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	GLSL::checkError(GET_FILE_LINE);
}

void ShapeSkin::parseAnimationFile(string filename) {
	ifstream in;
	in.open(filename);
	if (!in.good()) {
		cout << "Cannot read " << filename << endl;
		return;
	}
	cout << "Loading " << filename << endl;

	string line;
	while (true) {
		getline(in, line);
		if (in.eof()) {
			break;
		}
		if (line.empty()) {
			continue;
		}
		// Skip comments
		if (line.at(0) == '#') {
			continue;
		}
		break; //Once we get to actual data break out
	}
	string framesStr;
	string bonesStr;

	stringstream ss(line);

	ss >> framesStr;
	int frames = stoi(framesStr);
	sz = frames; //apparently there's 28 total bind positions, 1 t pose plus 27 frames (NOT 26 frames whoops)
	ss >> bonesStr;
	int bones = stoi(bonesStr);
	bns = bones;

	float floatArr[7];
	string number;

	animationFrames = new glm::mat4 * [sz];
	invertedAnimationFrames = new glm::mat4 * [sz];
	glm::mat4* boneMatrices;
	glm::mat4* invertedBoneMatrices;

	for (int i = -1; i < frames; i++) {
		getline(in, line);
		stringstream ss(line);
		boneMatrices = new glm::mat4[bones];
		invertedBoneMatrices = new glm::mat4[bones];

		for (int j = 0; j < bones; j++) {
			//read in 7 floats
			for (int k = 0; k < 7; k++) {
				ss >> number;
				floatArr[k] = stof(number);
			}

			//make the matrix
			glm::quat q(floatArr[3], floatArr[0], floatArr[1], floatArr[2]); //w x y z
			glm::mat4 E = glm::mat4_cast(q); //cast to matrix
			E[3] = glm::vec4(floatArr[4], floatArr[5], floatArr[6], 1.0f); //x y z 1.0f

			//put the matrix in the boneMatrix array
			boneMatrices[j] = E;
			invertedBoneMatrices[j] = inverse(E);
		}

		if (i == -1) { //first one is the bind pose
			bindPose = boneMatrices;
			invertedBindPose = invertedBoneMatrices;
		}
		else {
			animationFrames[i] = boneMatrices;
			invertedAnimationFrames[i] = invertedBoneMatrices;
		}
	}
	in.close();
}

void ShapeSkin::update(int k)
{
	// TODO: CPU skinning calculations.
	// After computing the new positions and normals, send the new data
	// to the GPU by copying and pasting the relevant code from the
	// init() function.
	mat4* bindAnimProducts = new mat4[bones]; //array of mat4s
	for (int j = 0; j < bones; j++) { //get the array of stuff
		bindAnimProducts[j] = animationFrames[k][j] * bindPose[j]; //might need to strip out the rotation? TBD
	}

	for (int i = 0; i < vertices; i++) { //vertex
		int ind = i * 3;
		vec4 initialPos = vec4(posBuf[ind], posBuf[ind + 1], posBuf[ind + 2], 1);
		vec4 initialNor = vec4(norBuf[ind], norBuf[ind + 1], norBuf[ind + 2], 1);

		vec4 skinnedPos(0, 0, 0, 1);
		vec4 skinnedNor(0, 0, 0, 0);
		for (int j = 0; j < maxInfluences; j++) {//bone influence
			float skinningWeight = boneWeightBuf[i * maxInfluences + j];
			if (skinningWeight <= 0.0f) {
				break; //no more bone influences past this point, so no need to finish influences for loop
			}
			int boneSelected = boneIndBuf[i * maxInfluences + j];
			skinnedPos += skinningWeight * bindAnimProducts[boneSelected] * initialPos;
			skinnedNor += skinningWeight * bindAnimProducts[boneSelected] * initialNor;
		}

		animPosBuf[ind] = skinnedPos.x;
		animPosBuf[ind + 1] = skinnedPos.y;
		animPosBuf[ind + 2] = skinnedPos.z;
	}

	// Send the position array to the GPU
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, animPosBuf.size() * sizeof(float), &animPosBuf[0], GL_DYNAMIC_DRAW);

	// Send the normal array to the GPU
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glBufferData(GL_ARRAY_BUFFER, animNorBuf.size() * sizeof(float), &animNorBuf[0], GL_DYNAMIC_DRAW);

	// Unbind the arrays
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	free(bindAnimProducts);

	GLSL::checkError(GET_FILE_LINE);
}

void ShapeSkin::draw(int k) const
{
	assert(prog);

	// Send texture matrix
	glUniformMatrix3fv(prog->getUniform("T"), 1, GL_FALSE, glm::value_ptr(T->getMatrix()));
	
	int h_pos = prog->getAttribute("aPos");
	glEnableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

	int h_nor = prog->getAttribute("aNor");
	glEnableVertexAttribArray(h_nor);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

	int h_tex = prog->getAttribute("aTex");
	glEnableVertexAttribArray(h_tex);
	glBindBuffer(GL_ARRAY_BUFFER, texBufID);
	glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	
	// Draw
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBufID);
	glDrawElements(GL_TRIANGLES, (int)elemBuf.size(), GL_UNSIGNED_INT, (const void *)0);
	
	glDisableVertexAttribArray(h_nor);
	glDisableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	GLSL::checkError(GET_FILE_LINE);
}
