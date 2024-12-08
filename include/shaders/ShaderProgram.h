#ifndef SHADER_PROGRAM_H
#define SHADER_PROGRAM_H

using namespace std;

class ShaderProgram {

public:

	ShaderProgram();
	~ShaderProgram();

	unsigned int programID;

	unsigned int vertexShaderID;
	unsigned int fragmentShaderID;
};


#endif