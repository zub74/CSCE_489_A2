#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

class Skinner {
private:
	int sz = 0;
	int bns = 0;
public:
	glm::mat4* bindPose;
	glm::mat4** animationFrames;
	void parseAnimationFile(std::string filename);
	int frames() { return sz; }
	int bones() { return bns; }

	~Skinner() {
		free(bindPose);

		for (int i = 0; i < sz; i++) {
			free(animationFrames[i]);
		}
		free(animationFrames);
	}
};


