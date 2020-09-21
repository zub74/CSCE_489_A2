#include "Skinning.h"

using namespace std;

void Skinner::parseAnimationFile(string filename) {
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

	animationFrames = new glm::mat4*[sz]; 
	glm::mat4* boneMatrices;

	for (int i = -1; i < frames; i++) {
		getline(in, line);
		stringstream ss(line);

		boneMatrices = new glm::mat4[bones];

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
		}

		if (i == -1) { //first one is the bind pose
			bindPose = boneMatrices;
		}
		else {
			animationFrames[i] = boneMatrices;
		}
	}
	in.close();
}