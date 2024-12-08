#include "MyGLCanvas.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

MyGLCanvas::MyGLCanvas(int x, int y, int w, int h, const char* l) : Fl_Gl_Window(x, y, w, h, l) {
	mode(FL_OPENGL3 | FL_RGB | FL_ALPHA | FL_DEPTH | FL_DOUBLE);

	// Scene
	parser = NULL;
	scene = NULL;

	if (parser != NULL) {
		delete parser;
		delete scene;
		parser = NULL;
		scene = NULL;
	}

	// Camera
	camera = NULL;
	scale = 1.0f;

	camera = new Camera();
	rotVec = glm::vec3(0.0f, 0.0f, 0.0f);
	eyePosition = glm::vec3(2.0f, 2.0f, 2.0f);
	camera->orientLookAt(eyePosition, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

	// Shape
	segmentsX = 20;
	segmentsY = 20;

	firstTime = true;

	myTextureManager = new TextureManager();
	myShaderManager = new ShaderManager();
	// myObjectPLY = new ply("./data/sphere.ply");
	// myEnvironmentPLY = new ply("./data/sphere.ply");
}

MyGLCanvas::~MyGLCanvas() {
	delete myTextureManager;
	delete myShaderManager;
	delete myObjectPLY;
	delete myEnvironmentPLY;
}

void MyGLCanvas::initShaders() {
	myTextureManager->loadTexture("environMap", "./data/ppm/sphere-map-market.ppm");
	myTextureManager->loadTexture("objectTexture", "./data/ppm/brick.ppm");

	myShaderManager->addShaderProgram("objectShaders", "shaders/330/object-vert.shader", "shaders/330/object-frag.shader");
	// myObjectPLY->buildArrays();
	// myObjectPLY->bindVBO(myShaderManager->getShaderProgram("objectShaders")->programID);

	myShaderManager->addShaderProgram("environmentShaders", "shaders/330/environment-vert.shader", "shaders/330/environment-frag.shader");
	// myEnvironmentPLY->buildArrays();
	// myEnvironmentPLY->bindVBO(myShaderManager->getShaderProgram("environmentShaders")->programID);
}

void MyGLCanvas::draw() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (!valid()) {  //this is called when the GL canvas is set up for the first time or when it is resized...
		printf("establishing GL context\n");

		glViewport(0, 0, w(), h());
		updateCamera(w(), h());
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

		/****************************************/
		/*          Enable z-buferring          */
		/****************************************/

		glEnable(GL_DEPTH_TEST);
		glPolygonOffset(1, 1);
		if (firstTime == true) {
			firstTime = false;
			initShaders();
		}
	}

	// Clear the buffer of colors in each bit plane.
	// bit plane - A set of bits that are on or off (Think of a black and white image)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawScene();
}

void MyGLCanvas::drawScene() {
	// glm::mat4 viewMatrix = glm::lookAt(eyePosition, lookatPoint, glm::vec3(0.0f, 1.0f, 0.0f));

	// viewMatrix = glm::rotate(viewMatrix, TO_RADIANS(rotWorldVec.x), glm::vec3(1.0f, 0.0f, 0.0f));
	// viewMatrix = glm::rotate(viewMatrix, TO_RADIANS(rotWorldVec.y), glm::vec3(0.0f, 1.0f, 0.0f));
	// viewMatrix = glm::rotate(viewMatrix, TO_RADIANS(rotWorldVec.z), glm::vec3(0.0f, 0.0f, 1.0f));

	// glm::mat4 modelMatrix = glm::mat4(1.0);
	// modelMatrix = glm::rotate(modelMatrix, TO_RADIANS(rotVec.x), glm::vec3(1.0f, 0.0f, 0.0f));
	// modelMatrix = glm::rotate(modelMatrix, TO_RADIANS(rotVec.y), glm::vec3(0.0f, 1.0f, 0.0f));
	// modelMatrix = glm::rotate(modelMatrix, TO_RADIANS(rotVec.z), glm::vec3(0.0f, 0.0f, 1.0f));
	// modelMatrix = glm::scale(modelMatrix, glm::vec3(scaleFactor, scaleFactor, scaleFactor));

	// glm::vec4 lookVec(0.0f, 0.0f, -1.0f, 0.0f);

	// glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	// glEnable(GL_TEXTURE_2D);
	// //Pass first texture info to our shader 
	// glActiveTexture(GL_TEXTURE0);
	// glBindTexture(GL_TEXTURE_2D, myTextureManager->getTextureID("environMap"));
	// glActiveTexture(GL_TEXTURE1);
	// glBindTexture(GL_TEXTURE_2D, myTextureManager->getTextureID("objectTexture"));

	// //first draw the object sphere
	// glUseProgram(myShaderManager->getShaderProgram("objectShaders")->programID);

	// //TODO: add variable binding

	// myObjectPLY->renderVBO(myShaderManager->getShaderProgram("objectShaders")->programID);



	// //second draw the enviroment sphere
	// glUseProgram(myShaderManager->getShaderProgram("environmentShaders")->programID);

	// //TODO: add variable binding

	// myEnvironmentPLY->renderVBO(myShaderManager->getShaderProgram("environmentShaders")->programID);
}


void MyGLCanvas::updateCamera(int width, int height) {
	float xy_aspect;
	xy_aspect = (float)width / (float)height;

	camera->setScreenSize(width, height);
}


int MyGLCanvas::handle(int e) {
	//static int first = 1;
#ifndef __APPLE__
	if (firstTime && e == FL_SHOW && shown()) {
		firstTime = 0;
		make_current();
		GLenum err = glewInit(); // defines pters to functions of OpenGL V 1.2 and above
		if (GLEW_OK != err) {
			/* Problem: glewInit failed, something is seriously wrong. */
			fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		}
		else {
			//SHADER: initialize the shader manager and loads the two shader programs
			initShaders();
		}
	}
#endif	
	//printf("Event was %s (%d)\n", fl_eventnames[e], e);
	switch (e) {
	case FL_DRAG:
	case FL_MOVE:
	case FL_PUSH:
	case FL_RELEASE:
	case FL_KEYUP:
	case FL_MOUSEWHEEL:
		break;
	}
	return Fl_Gl_Window::handle(e);
}

void MyGLCanvas::resize(int x, int y, int w, int h) {
	Fl_Gl_Window::resize(x, y, w, h);
	puts("resize called");
}

void MyGLCanvas::reloadShaders() {
	myShaderManager->resetShaders();

	myShaderManager->addShaderProgram("objectShaders", "shaders/330/object-vert.shader", "shaders/330/object-frag.shader");
	// myObjectPLY->bindVBO(myShaderManager->getShaderProgram("objectShaders")->programID);

	myShaderManager->addShaderProgram("environmentShaders", "shaders/330/environment-vert.shader", "shaders/330/environment-frag.shader");
	// myEnvironmentPLY->bindVBO(myShaderManager->getShaderProgram("environmentShaders")->programID);

	invalidate();
}

// Load file
void MyGLCanvas::loadSceneFile(const char* filenamePath) {
	if (parser != NULL) {
		delete parser;
		delete scene;
	}
	parser = new SceneParser(filenamePath);

	bool success = parser->parse();
	cout << "success? " << success << endl;
	if (success == false) {
		delete parser;
		delete scene;
		parser = NULL;
		scene = NULL;
	}
	else {
		SceneCameraData cameraData;
		parser->getCameraData(cameraData);

		camera->reset();
		camera->setViewAngle(cameraData.heightAngle);
		if (cameraData.isDir == true) {
			camera->orientLookVec(cameraData.pos, cameraData.look, cameraData.up);
		}
		else {
			camera->orientLookAt(cameraData.pos, cameraData.lookAt, cameraData.up);
		}
		// parsing scene tree and flatten it
		this->scene = new SceneGraph();
        flatSceneData();
		this->scene->buildKDTree();
	}
}

void MyGLCanvas::flatSceneData() {
    flatSceneDataRec(parser->getRootNode(), glm::mat4(1.0f));
    this->scene->calculate();
}

void MyGLCanvas::flatSceneDataRec(SceneNode* node, glm::mat4 curMat) {
    for (SceneTransformation* transform : node->transformations) {
        switch (transform->type) {
            case TRANSFORMATION_SCALE:
                curMat = glm::scale(curMat, transform->scale);
                break;
            case TRANSFORMATION_ROTATE:
                curMat = glm::rotate(curMat, transform->angle, transform->rotate);
                break;
            case TRANSFORMATION_TRANSLATE:
                curMat = glm::translate(curMat, transform->translate);
                break;
            case TRANSFORMATION_MATRIX:
                curMat = curMat * transform->matrix;
                break;
        }
    }
    for (ScenePrimitive* primitive : node->primitives){
        switch (primitive->type) {
            case SHAPE_CUBE:
                this->scene->addNode(new SceneGraphNode(curMat, new Cube(), primitive->material));
                break;
            case SHAPE_CYLINDER:
                this->scene->addNode(new SceneGraphNode(curMat, new Cylinder(), primitive->material));
                break;
            case SHAPE_CONE:
                this->scene->addNode(new SceneGraphNode(curMat, new Cone(), primitive->material));
                break;
            case SHAPE_SPHERE:
                this->scene->addNode(new SceneGraphNode(curMat, new Sphere(), primitive->material));
                break;
            case SHAPE_SPECIAL1:
                this->scene->addNode(new SceneGraphNode(curMat, new Torus(), primitive->material));
                break;
            default:
                this->scene->addNode(new SceneGraphNode(curMat, new Cube(), primitive->material));
        }
    }
    if (node->children.size() == 0) {
        return;
    }
    else {
        for (SceneNode* child : node->children){
            flatSceneDataRec(child, curMat);
        }
    }
}

void MyGLCanvas::setSegments() {
	// set segments to be 20 for now
	Shape::setSegments(20, 20);
	if(this->scene != NULL) {
		this->scene->calculate();
	}
}

void MyGLCanvas::bindScene() {
	GLuint tbo, texture;
	glGenBuffers(1, &tbo);
	glBindBuffer(GL_TEXTURE_BUFFER, tbo);

	std::vector<float> array;
	this->scene->buildArray(array);

	glBufferData(GL_TEXTURE_BUFFER, array.size() * sizeof(float), array.data(), GL_STATIC_DRAW);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_BUFFER, texture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, tbo);
}
