#pragma once
#ifndef TEXTURE_MATRIX_H
#define TEXTURE_MATRIX_H

#include <glm/fwd.hpp>

class TextureMatrix
{
public:
	enum Type
	{
		BODY,
		MOUTH,
		EYES,
		BROWS,
		NONE
	};
	
	TextureMatrix();
	virtual ~TextureMatrix();
	void setType(const std::string &str);
	void update(unsigned int key);
	glm::mat3 getMatrix() const { return T; }
	
private:
	Type type;
	glm::mat3 T;
	int eHor = 0; //keep track of the number of times it's been pressed to mod it 
	int eVer = 0;
	int mHor = 0;
	int mVer = 0;
	int bVer = 0;
};

#endif
