#pragma once
#ifndef SHAPESKIN_H
#define SHAPESKIN_H

#include <memory>

#define GLEW_STATIC
#include <GL/glew.h>

class MatrixStack;
class Program;
class TextureMatrix;

class ShapeSkin
{
public:
	ShapeSkin();
	virtual ~ShapeSkin();
	void setTextureMatrixType(const std::string &meshName);
	void loadMesh(const std::string &meshName);
	void loadAttachment(const std::string &filename);
	void setProgram(std::shared_ptr<Program> p) { prog = p; }
	void init();
	void update(int k);
	void draw(int k) const;
	void setTextureFilename(const std::string &f) { textureFilename = f; }
	std::string getTextureFilename() const { return textureFilename; }
	std::shared_ptr<TextureMatrix> getTextureMatrix() { return T; }

	glm::mat4* bindPose;
	glm::mat4* invertedBindPose;
	glm::mat4** animationFrames;
	glm::mat4** invertedAnimationFrames;
	void parseAnimationFile(std::string filename);
	int frames() { return sz; }
	int numBones() { return bns; }
	
private:
	std::shared_ptr<Program> prog;
	std::vector<unsigned int> elemBuf;
	std::vector<float> posBuf;
	std::vector<float> animPosBuf;
	std::vector<float> norBuf;
	std::vector<float> animNorBuf;
	std::vector<float> texBuf;
	std::vector<int> boneIndBuf;
	std::vector<float> boneWeightBuf;
	int vertices;
	int bones;
	int maxInfluences;
	int sz;
	int bns;
	GLuint elemBufID;
	GLuint posBufID;
	GLuint animPosBufID;
	GLuint norBufID;
	GLuint animNorBufID;
	GLuint texBufID;
	std::string textureFilename;
	std::shared_ptr<TextureMatrix> T;
};

#endif
