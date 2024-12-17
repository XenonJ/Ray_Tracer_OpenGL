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

#include <unistd.h>
#include <limits.h>

class MyGLCanvas : public Fl_Gl_Window {
public:

	// Frame counter
	int frameCounter = 0;

	// Cloud Parameters
	float cloudDensity;
	float cloudSpeed;
	float cloudWidth;
	float cloudBottom;
	float cloudTop;
	float sampleRange;

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
	glm::vec3 meshTranslate;	// translation for mesh rendering
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

	void bindMesh(std::vector<float>& array);
	void bindScene();
	void bindPLY(glm::mat4 mat);
	void bindKDTree(std::vector<float>& array);
	void initializeFBO(int width, int height);
	void resizeFBO(int width, int height);
	void readFBOData(int width, int height);

	void loadPLY(std::string filename);
	void loadPlane();
	void initShaders();
	void loadNoise(std::string filename);
	TextureManager* getTextureManager() { return myTextureManager; }

private:
	void draw();
	void drawScene();
	
	void flatSceneData();
	void flatSceneDataRec(SceneNode* node, glm::mat4 curMat);

	

	int handle(int);
	void resize(int x, int y, int w, int h);
	void updateCamera(int width, int height);

	// vertex buffer
	void initializeVertexBuffer();
	
	// Worley points

	// Cell size for worley noise
	float numCellsPerAxis;
	std::vector<glm::vec3> worleyPoints;
	std::vector<glm::vec3> CreateWorleyPoints(int numCellsPerAxis);
	void updateWorleyPoints(int numCellsPerAxis);


	GLuint vao;
	GLuint vbo;
	std::vector<float> pixelIndices;
	GLuint colorTexID;
	GLuint distanceTexID;
	GLuint fbo;

	// noise texture
	ppm* noiseTex;

	// texture buffer
	std::vector<GLuint> meshTextureBuffers;
	std::vector<GLuint> treeTextureBuffers;
	int rootIndex;	// index for kdtree root node
	int meshSize;	// mesh triangle number
	int treeSize;	// mesh kdtree node number
	size_t maxBufferSize = 16 * 1024 * 1024; // 16MB
	size_t floatsPerTriangle = 18; // 18 float for a mesh
	size_t floatsPerNode = 15;	// 15 float for a kdtree node

	TextureManager* myTextureManager;
	ShaderManager* myShaderManager;
	ply* myObjectPLY;

	glm::mat4 perspectiveMatrix;
	bool firstTime;
};

#endif // !MYGLCANVAS_H