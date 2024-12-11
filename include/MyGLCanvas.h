#pragma once

#ifndef MYGLCANVAS_H
#define MYGLCANVAS_H

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
#include <glm/glm.hpp>
#include <time.h>
#include <iostream>

#include "shaders/TextureManager.h"
#include "shaders/ShaderManager.h"
#include "shaders/ply.h"
#include "gfxDefs.h"

#include "./objects/Cube.h"
#include "./objects/Cone.h"
#include "./objects/Cylinder.h"
#include "./objects/Sphere.h"
#include "./objects/Torus.h"

#include "./objects/SceneGraph.h"
#include "scene/Camera.h"


class MyGLCanvas : public Fl_Gl_Window {
public:

	// Frame counter
	int frameCounter = 0;

	// Camera
	float scale;
	Camera* camera;

	// Scene
	SceneParser* parser;
	SceneGraph* scene;

	// Shape
	int segmentsX;
	int segmentsY;

	// Length of our spline (i.e how many points do we randomly generate)


	glm::vec3 eyePosition;
	glm::vec3 rotVec;
	// glm::vec3 lookatPoint;
	// glm::vec3 lightPos;
	// glm::vec3 rotWorldVec;

	// int useDiffuse;
	// float lightAngle; //used to control where the light is coming from
	// int viewAngle;
	// float clipNear;
	// float clipFar;
	// float scaleFactor;
	// float textureBlend;

	MyGLCanvas(int x, int y, int w, int h, const char* l = 0);
	~MyGLCanvas();

	// void loadPLY(std::string filename);
	// void loadEnvironmentTexture(std::string filename);
	// void loadObjectTexture(std::string filename);
	void reloadShaders();
	void setSegments();
	void loadSceneFile(const char* filenamePath);

	void bindScene();

private:
	void draw();
	void drawScene();
	
	void flatSceneData();
	void flatSceneDataRec(SceneNode* node, glm::mat4 curMat);

	void initShaders();

	int handle(int);
	void resize(int x, int y, int w, int h);
	void updateCamera(int width, int height);

	// vertex buffer
	void initializeVertexBuffer();
	GLuint vao;
	GLuint vbo;
	std::vector<float> pixelIndices;

	// texture buffer
	GLuint meshTextureBuffer;
	int meshSize;

	TextureManager* myTextureManager;
	ShaderManager* myShaderManager;
	ply* myObjectPLY;

	glm::mat4 perspectiveMatrix;

	bool firstTime;
};

#endif // !MYGLCANVAS_H