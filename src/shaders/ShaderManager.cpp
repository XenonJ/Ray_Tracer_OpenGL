/*  =================== File Information =================
	File Name: shaderManager.cpp
	Description:
	Author: Michael Shah
	Last Modified: 4/14/14

	Purpose: A Class for loading in shaders to use
	Usage:	

	Further Reading resources: 
	===================================================== */
#include <iostream>
#include <string>
#include <fstream>
#if defined(__APPLE__)
#  include <OpenGL/gl3.h> // defines OpenGL 3.0+ functions
#else
#  if defined(WIN32)
#    define GLEW_STATIC 1
#  endif
#  include <GL/glew.h>
#endif
#include <FL/glut.h>
#include <FL/glu.h>
#include "shaders/ppm.h"
#include "shaders/ShaderManager.h"

using namespace std;

	/*	===============================================
	Desc:
	Precondition: 
	Postcondition:
	=============================================== */ 
ShaderManager::ShaderManager(){
		// Return the version of OpenGL you are running.
#ifndef __APPLE__
		fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
#endif
	}
	/*	===============================================
	Desc:	Here we clean our program up
	Precondition: 
	Postcondition:
	=============================================== */ 
ShaderManager::~ShaderManager(){
	resetShaders();
}

void ShaderManager::resetShaders() {
	for (auto const& it : shaderPrograms) {
		delete it.second;  //delete the memory of the ShaderProgram object
	}
	shaderPrograms.clear();
}

/*	===============================================
Desc: 	Takes a file and essentially converts it into a 
		single string that can be compiled by OpenGL at runtime.
Precondition: 
Postcondition:
=============================================== */ 
void ShaderManager::loadFile(const char* fileName, string& str){
	ifstream inputFile(fileName);
	// If our file does not open, print out an error
	// otherwise process it.
	if(!inputFile.is_open()){
		cout << "The file " << fileName << " cannot be open." << endl;
		return;
	}
	// Process a line of characters with a max size of 300 characters in a line.
	const int lineSize = 300;
	char buffer[lineSize];
	cout << "Processing shader file: " << fileName << endl;
	cout << "========================" << endl;
 	// Process each line of the shader and store it in a string.
	// Remember, that shaders are just strings that get compiled
	// at the runtime to be run within our program.
	// Because they get compiled at runtime, we do not need to compile
	// the rest of our program either!
	while(!inputFile.eof()){
		inputFile.getline(buffer,lineSize);
		str+=buffer;
		str+='\n';
	}
}

/*	===============================================
Desc: 	The source is a string that has been loaded from our
		loadFile function, or otherwise it is a complete OpenGL shader
		that has been compressed into a single string.

		The mode tells us if it is a vertex or fragment shader.
		The unsigned int values for mode are GL_VERTEX_SHADER or GL_FRAGMENT_SHADER

		The id that is returned, is a unique id that openGL assigns from
		the glCreateShader command.  OpenGL needs this in order to reference
		the correct shader.
Precondition: 
Postcondition:
=============================================== */ 
unsigned int ShaderManager::loadShader(string& source, unsigned int mode){
	// The unique id for our shader
	unsigned int id;
	// Create the shader, and tell us if it is a vertex, fragment, geometry, or tesselation shader.
	id = glCreateShader(mode);

	// We create a c-string which will get passed into the glShaderSource command.
	const char* csource = source.c_str();
	// The command 'glShaderSource' takes in the parameters in the order below.
	// shader - the handler to the shader
	// numOfStrings - Number of strings in the array
	// strings - the array of strings
	// lenOfStrings - An array with the length of each string, or NULL, if the strings
	//				  are NULL terminated.
	glShaderSource(id,1,&csource,NULL);
	// Once we have our shader, we have to compile it at runtime
	glCompileShader(id);
	// In case there are errors, we need to output them to the console.
	// Allocate memory for a buffer to hold error messages.
	char error[1024];
	glGetShaderInfoLog(id,1024,NULL,error);
	cout << "Compiler errors: " << endl << error << endl;

	return id;
}

/*	===============================================
Desc:	Currently we are working with vertex and fragment shaders

		So we can initialize them here.

Precondition: 
Postcondition:
=============================================== */ 
void ShaderManager::addShaderProgram(const char* programName, const char* vertexShaderName, const char* fragmentShaderName){

	ShaderProgram* program = new ShaderProgram();

	// Create a variable to hold our source
	string source;
	// Load our source with a vertex shader
	loadFile(vertexShaderName,source);
	// Compile the shader and assign it the unique id of our vertexShaderID
	program->vertexShaderID = loadShader(source, GL_VERTEX_SHADER);

	// We then repeat the process for the fragment shader

	// Load our source for a fragment shader
	// Because source is a reference, we need to clear it with the previous vertexShader
	// strings loaded from the previous 'loadFile' call.
	source="";
	loadFile(fragmentShaderName,source);
	// Compile our fragment shader
	program->fragmentShaderID = loadShader(source, GL_FRAGMENT_SHADER);

	// Now that we've compiled the actual shaders, we add them to our program.
	// We can create as many programs as we want and then switch between them.
	program->programID = glCreateProgram();
	glAttachShader(program->programID, program->vertexShaderID);
	glAttachShader(program->programID, program->fragmentShaderID);

	// Finally we link them, this is the same as the linking process for C++
	// with a library file
	glLinkProgram(program->programID);
	// Now we finally decide to use the program
	//glUseProgram(program->programID);

	shaderPrograms[programName] = program;
}

ShaderProgram* ShaderManager::getShaderProgram(std::string name) {
	return shaderPrograms[name];
}
