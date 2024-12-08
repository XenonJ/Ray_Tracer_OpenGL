#include "shaders/ShaderProgram.h"

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


ShaderProgram::ShaderProgram() {
	programID = -1;
	vertexShaderID = -1;
	fragmentShaderID = -1;
}

ShaderProgram::~ShaderProgram() {
	if (programID != -1) {
		glDetachShader(programID, vertexShaderID);
		glDetachShader(programID, fragmentShaderID);
	}

	if (vertexShaderID != -1) {
		glDeleteShader(vertexShaderID);
	}
	if (fragmentShaderID != -1) {
		glDeleteShader(fragmentShaderID);
	}

	if (programID != -1) {
		glDeleteProgram(programID);
	}
}