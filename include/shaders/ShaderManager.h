/*  =================== File Information =================
	File Name: shaderManager.h
	Description:
	Author: Michael Shah
	Last Modified: 4/14/14

	Purpose: A Class for loading in shaders to use
	Usage:	

	Further Reading resources: 
	===================================================== */
#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#include "ppm.h"
#include "ShaderProgram.h"
#include <map>

using namespace std;

class ShaderManager{

	public:

	ShaderManager();
	~ShaderManager();

	void resetShaders();

	/*	===============================================
	Desc: 	Takes a file and essentially converts it into a 
			single string that can be compiled by OpenGL at runtime.
	Precondition: 
	Postcondition:
	=============================================== */ 
	void loadFile(const char* fileName, string& str);

	unsigned int loadShader(string& source, unsigned int mode);

	void addShaderProgram(const char* programName, const char* vertexShaderName, const char* fragmentShaderName);

	ShaderProgram* getShaderProgram(std::string name);

	private:
		std::map<std::string, ShaderProgram*> shaderPrograms;
};


#endif