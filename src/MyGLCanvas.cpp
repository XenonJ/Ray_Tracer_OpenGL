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
	eyePosition = glm::vec3(20.0f, 20.0f, 20.0f);
	camera->orientLookAt(eyePosition, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

	// Shape
	segmentsX = 3;
	segmentsY = 3;

	firstTime = true;

	myTextureManager = new TextureManager();
	myShaderManager = new ShaderManager();
	// myObjectPLY = new ply("./data/sphere.ply");

	// initialize worley points
	numCellsPerAxis = 10;
	worleyPoints = CreateWorleyPoints(numCellsPerAxis);

	// noiseTex = nullptr;
	// initialize cloud data
	cloudDensity = 1.0f;
	cloudSpeed = 2.0f;
	cloudWidth = 250.0f;
	cloudBottom = 1.0f;
	cloudTop = 5.0f;
	sampleRange = 50.0f;
}

MyGLCanvas::~MyGLCanvas() {
	delete myTextureManager;
	delete myShaderManager;
	delete myObjectPLY;
	// delete noiseTex;
}

void MyGLCanvas::initShaders() {
	printf("init shaders\n");
	myTextureManager->loadTexture3D("noiseTex", "./data/ppm/tiled_worley_noise.ppm");
	myTextureManager->loadTexture("seaTex", "./data/ppm/sea1.ppm");
	myTextureManager->loadTexture("seaNormalTex", "./data/ppm/sea1_normal.ppm");
	myShaderManager->addShaderProgram("objectShaders", "shaders/330/object-vert.shader", "shaders/330/object-frag.shader");
	myShaderManager->addShaderProgram("environmentShaders", "shaders/330/environment-vert.shader", "shaders/330/environment-frag.shader");
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
			initializeVertexBuffer();
			initializeFBO(w(), h());
		}
	}

	// Clear the buffer of colors in each bit plane.
	// bit plane - A set of bits that are on or off (Think of a black and white image)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawScene();
}

// Generate Worley points buffer
std::vector<glm::vec3> MyGLCanvas::CreateWorleyPoints(int numCellsPerAxis) {
    std::vector<glm::vec3> points;
    float cellSize = 1.0f / numCellsPerAxis;

    for (int x = 0; x < numCellsPerAxis; x++) {
        for (int y = 0; y < numCellsPerAxis; y++) {
            for (int z = 0; z < numCellsPerAxis; z++) {
                glm::vec3 randomOffset = glm::vec3(
                    static_cast<float>(rand()) / RAND_MAX,
                    static_cast<float>(rand()) / RAND_MAX,
                    static_cast<float>(rand()) / RAND_MAX
                );
                glm::vec3 position = glm::vec3(x, y, z) + randomOffset * cellSize;
                points.push_back(position);
            }
        }
    }
    return points;
}

// Update worley points
void MyGLCanvas::updateWorleyPoints(int numCellsPerAxis) {
	worleyPoints = CreateWorleyPoints(numCellsPerAxis);
	// pass worley points
	GLuint worleyTBO, worleyTexture;
	glGenBuffers(2, &worleyTBO);
	glBindBuffer(GL_TEXTURE_BUFFER, worleyTBO);
	glBufferData(GL_TEXTURE_BUFFER, worleyPoints.size() * sizeof(glm::vec3), worleyPoints.data(), GL_STATIC_DRAW);
	glGenTextures(2, &worleyTexture);
	glBindTexture(GL_TEXTURE_BUFFER, worleyTexture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, worleyTBO);
	// GLint worleyPointsLoc = glGetUniformLocation(myShaderManager->getShaderProgram("objectShaders")->programID, "worleyPoints");
	// glUniform1i(worleyPointsLoc, 0);
}

void MyGLCanvas::drawScene() {
	// incr frame counter
	frameCounter++;
	if (frameCounter == INT_MAX)
	{
		frameCounter = 0;
	}

	// make object shaders output into fbo
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glUseProgram(myShaderManager->getShaderProgram("objectShaders")->programID);

    // bind vao
    glBindVertexArray(vao);

    // pass Uniform
    GLint eyeLoc = glGetUniformLocation(myShaderManager->getShaderProgram("objectShaders")->programID, "eyePosition");
    GLint lookVecLoc = glGetUniformLocation(myShaderManager->getShaderProgram("objectShaders")->programID, "lookVec");
    GLint upVecLoc = glGetUniformLocation(myShaderManager->getShaderProgram("objectShaders")->programID, "upVec");
    GLint viewAngleLoc = glGetUniformLocation(myShaderManager->getShaderProgram("objectShaders")->programID, "viewAngle");
    GLint nearLoc = glGetUniformLocation(myShaderManager->getShaderProgram("objectShaders")->programID, "nearPlane");
    GLint widthLoc = glGetUniformLocation(myShaderManager->getShaderProgram("objectShaders")->programID, "screenWidth");
    GLint heightLoc = glGetUniformLocation(myShaderManager->getShaderProgram("objectShaders")->programID, "screenHeight");
    GLint lightPosLoc = glGetUniformLocation(myShaderManager->getShaderProgram("objectShaders")->programID, "lightPos");
    GLint meshTransLoc = glGetUniformLocation(myShaderManager->getShaderProgram("objectShaders")->programID, "meshTrans");

	// pass camera data
	glUniform3fv(eyeLoc, 1, glm::value_ptr(camera->getEyePoint()));
	glUniform3fv(lookVecLoc, 1, glm::value_ptr(camera->getLookVector()));
	glUniform3fv(upVecLoc, 1, glm::value_ptr(camera->getUpVector()));
	glUniform1f(viewAngleLoc, camera->getViewAngle());
	glUniform1f(nearLoc, camera->getNearPlane());
	glUniform1f(widthLoc, camera->getScreenWidth());
	glUniform1f(heightLoc, camera->getScreenHeight());
	glUniform3fv(lightPosLoc, 1, glm::value_ptr(glm::vec3(300.0f)));	// default light
	glUniform3fv(meshTransLoc, 1, glm::value_ptr(meshTranslate));	// mesh translation

	glUniform1i(glGetUniformLocation(myShaderManager->getShaderProgram("objectShaders")->programID, "frameCounter"), frameCounter);

	// pass scene data
	if (this->parser || this->myObjectPLY) {
		// pass texture buffers
		for (size_t i = 0; i < this->meshTextureBuffers.size(); ++i) {
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_BUFFER, this->meshTextureBuffers[i]);
			std::string uniformName = "meshBuffer[" + std::to_string(i) + "]";
			GLuint location = glGetUniformLocation(myShaderManager->getShaderProgram("objectShaders")->programID, uniformName.c_str());
			glUniform1i(location, i); // Bind texture to the corresponding uniform
		}
    	GLint numMeshBuffersLoc = glGetUniformLocation(myShaderManager->getShaderProgram("objectShaders")->programID, "numMeshBuffers");
    	GLint maxTrianglesPerBufferLoc = glGetUniformLocation(myShaderManager->getShaderProgram("objectShaders")->programID, "maxTrianglesPerBuffer");
		glUniform1i(numMeshBuffersLoc, this->meshTextureBuffers.size());
		size_t maxTrianglesPerBuffer = maxBufferSize / (floatsPerTriangle * sizeof(float));
		glUniform1i(maxTrianglesPerBufferLoc, int(maxTrianglesPerBuffer));
		GLint meshSizeLoc = glGetUniformLocation(myShaderManager->getShaderProgram("objectShaders")->programID, "meshSize");
		glUniform1i(meshSizeLoc, meshSize);

		// pass kdtree array
		size_t startTextureUnit = this->meshTextureBuffers.size(); // Offset by the number of mesh buffers
		for (size_t i = 0; i < this->treeTextureBuffers.size(); ++i) {
			glActiveTexture(GL_TEXTURE0 + startTextureUnit + i); // Start from the next available texture unit
			glBindTexture(GL_TEXTURE_BUFFER, this->treeTextureBuffers[i]);
			std::string uniformName = "treeBuffer[" + std::to_string(i) + "]";
			GLuint location = glGetUniformLocation(myShaderManager->getShaderProgram("objectShaders")->programID, uniformName.c_str());
			glUniform1i(location, startTextureUnit + i); // Bind texture to the corresponding uniform
		}
		GLint numTreeBuffersLoc = glGetUniformLocation(myShaderManager->getShaderProgram("objectShaders")->programID, "numTreeBuffers");
    	GLint maxNodesPerBufferLoc = glGetUniformLocation(myShaderManager->getShaderProgram("objectShaders")->programID, "maxNodesPerBuffer");
    	GLint rootIndexLoc = glGetUniformLocation(myShaderManager->getShaderProgram("objectShaders")->programID, "rootIndex");
		glUniform1i(numTreeBuffersLoc, this->treeTextureBuffers.size());
		size_t maxNodesPerBuffer = maxBufferSize / (floatsPerNode * sizeof(float));
		glUniform1i(maxNodesPerBufferLoc, int(maxNodesPerBuffer));
		glUniform1i(rootIndexLoc, rootIndex);
		GLint treeSizeLoc = glGetUniformLocation(myShaderManager->getShaderProgram("objectShaders")->programID, "treeSize");
		glUniform1i(treeSizeLoc, treeSize);

		// pass light
		SceneLightData lightData;
		if (parser && parser->getLightData(0, lightData)) {
			glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightData.pos));
		}
	}

    // draw pixels
    glDrawArrays(GL_POINTS, 0, w() * h());

	// readFBOData(w(), h());

	// release depth buffer and frame buffer
	glClear(GL_DEPTH_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// draw environment
	glUseProgram(myShaderManager->getShaderProgram("environmentShaders")->programID);

	// bind vao
    glBindVertexArray(vao);

	// bind noise texture
    // glActiveTexture(GL_TEXTURE0);
    // noiseTex->bindTexture();

    // // pass texture to shader
    // GLuint programID = myShaderManager->getShaderProgram("environmentShaders")->programID;
    // GLint textureUniformLoc = glGetUniformLocation(programID, "noiseTex");
    // glUniform1i(textureUniformLoc, 0); // bing to GL_TEXTURE0

	// use fbo
	// bind color tex to GL_TEXTURE1
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, colorTexID);
	GLint colorMapLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "colorMap");
	glUniform1i(colorMapLoc, 1);

	// bind distance tex to GL_TEXTURE2
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, distanceTexID);
	GLint distanceMapLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "distanceMap");
	glUniform1i(distanceMapLoc, 2);

	// bind sea tex to GL_TEXTURE3
	GLint seaTexLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "seaTex");
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, myTextureManager->getTextureID("seaTex"));
    glUniform1i(seaTexLoc, 3);

	// bind sea tex to GL_TEXTURE4
    GLint seaNormalTexLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "seaNormalTex");
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, myTextureManager->getTextureID("seaNormalTex"));
    glUniform1i(seaNormalTexLoc, 4);

    // pass Uniform
    eyeLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "eyePosition");
    lookVecLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "lookVec");
    upVecLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "upVec");
    viewAngleLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "viewAngle");
    nearLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "nearPlane");
    widthLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "screenWidth");
    heightLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "screenHeight");
    lightPosLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "lightPos");
	GLint framebufferSizeLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "framebufferSize");
	// pass camera data
	glUniform3fv(eyeLoc, 1, glm::value_ptr(camera->getEyePoint()));
	glUniform3fv(lookVecLoc, 1, glm::value_ptr(camera->getLookVector()));
	glUniform3fv(upVecLoc, 1, glm::value_ptr(camera->getUpVector()));
	glUniform1f(viewAngleLoc, camera->getViewAngle());
	glUniform1f(nearLoc, camera->getNearPlane());
	glUniform1f(widthLoc, camera->getScreenWidth());
	glUniform1f(heightLoc, camera->getScreenHeight());
	glUniform3fv(lightPosLoc, 1, glm::value_ptr(glm::vec3(300.0f)));	// default light
	glUniform2f(framebufferSizeLoc, float(w()), float(h()));

	glUniform1i(glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "frameCounter"), frameCounter);
	// pass cloud data
	GLint cloudDensityLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "cloudDensity");
	GLint cloudSpeedLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "cloudSpeed");
	GLint cloudWidthLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "width");
	GLint cloudBottomLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "bottom");
	GLint cloudTopLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "top");
	GLint sampleRangeLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "sampleRange");

	// pass cloud data
	glUniform1f(cloudDensityLoc, cloudDensity);
	glUniform1f(cloudSpeedLoc, cloudSpeed);
	glUniform1f(cloudWidthLoc, cloudWidth);
	glUniform1f(cloudBottomLoc, cloudBottom);
	glUniform1f(cloudTopLoc, cloudTop);
	glUniform1f(sampleRangeLoc, sampleRange);

    glDrawArrays(GL_POINTS, 0, w() * h());
	

    // release
    glBindVertexArray(0);
    glUseProgram(0);
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
	initializeVertexBuffer();
	resizeFBO(w, h);
	puts("resize called");
}

void MyGLCanvas::reloadShaders() {
	myShaderManager->resetShaders();

	myShaderManager->addShaderProgram("objectShaders", "shaders/330/object-vert.shader", "shaders/330/object-frag.shader");
	myShaderManager->addShaderProgram("environmentShaders", "shaders/330/environment-vert.shader", "shaders/330/environment-frag.shader");
	// myObjectPLY->bindVBO(myShaderManager->getShaderProgram("objectShaders")->programID);

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
		updateCamera(w(), h());
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
		this->bindScene();
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
	Shape::setSegments(this->segmentsX, this->segmentsY);
	printf("setting segments to %d, %d\n", this->segmentsX, this->segmentsY);
	if(this->scene != NULL) {
		this->scene->calculate();
	}
}

void MyGLCanvas::bindMesh(std::vector<float>& array) {
	// calculate size
	size_t maxTrianglesPerBuffer = maxBufferSize / (floatsPerTriangle * sizeof(float));
	size_t totalTriangles = array.size() / floatsPerTriangle;

	std::vector<GLuint> tboList, textureList;

	size_t start = 0;

	while (start < totalTriangles) {
        size_t end = std::min(start + maxTrianglesPerBuffer, totalTriangles);
        size_t bufferSize = (end - start) * floatsPerTriangle;

        // Create and upload buffer
        GLuint tbo, texture;
        glGenBuffers(1, &tbo);
        glBindBuffer(GL_TEXTURE_BUFFER, tbo);

        glBufferData(GL_TEXTURE_BUFFER, bufferSize * sizeof(float), &array[start * floatsPerTriangle], GL_STATIC_DRAW);

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_BUFFER, texture);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, tbo);

        // Store buffer and texture IDs
        tboList.push_back(tbo);
        textureList.push_back(texture);

        start = end;
    }

	this->meshTextureBuffers = textureList;
	this->meshSize = totalTriangles;
	printf("Total buffers: %lu, Total triangles: %lu\n", textureList.size(), totalTriangles);
}

void MyGLCanvas::bindKDTree(std::vector<float>& array) {
	// calculate size
	size_t maxNodesPerBuffer = maxBufferSize / (floatsPerNode * sizeof(float));
	size_t totalNodes = array.size() / floatsPerNode;

	std::vector<GLuint> tboList, textureList;

	size_t start = 0;

	while (start < totalNodes) {
        size_t end = std::min(start + maxNodesPerBuffer, totalNodes);
        size_t bufferSize = (end - start) * floatsPerNode;

        // Create and upload buffer
        GLuint tbo, texture;
        glGenBuffers(1, &tbo);
        glBindBuffer(GL_TEXTURE_BUFFER, tbo);

        glBufferData(GL_TEXTURE_BUFFER, bufferSize * sizeof(float), &array[start * floatsPerNode], GL_STATIC_DRAW);

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_BUFFER, texture);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, tbo);

        // Store buffer and texture IDs
        tboList.push_back(tbo);
        textureList.push_back(texture);

        start = end;
		printf("Uploading buffer size: %lu bytes\n", bufferSize * sizeof(float));
    }

	this->treeTextureBuffers = textureList;
	this->treeSize = totalNodes;
	printf("Total buffers: %lu, Total nodes: %lu\n", textureList.size(), totalNodes);
}

void printTreeBuffer(GLuint treeBufferID, size_t bufferSize) {
    // Bind the buffer
    glBindBuffer(GL_TEXTURE_BUFFER, treeBufferID);

    // Allocate a local array to store the buffer data
    std::vector<float> bufferData(bufferSize);

    // Retrieve the data from the buffer
    glGetBufferSubData(GL_TEXTURE_BUFFER, 0, bufferSize * sizeof(float), bufferData.data());

    // Print the buffer content
    printf("Contents of treeBuffer:\n");
    for (size_t i = 0; i < bufferSize; i += 3) { // Assuming vec3 (GL_RGB32F)
        printf("[%zu]: %f %f %f\n", i / 3, bufferData[i], bufferData[i + 1], bufferData[i + 2]);
    }

    // Unbind the buffer
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
}

// bind scene meshes into gl texture buffer
void MyGLCanvas::bindScene() {
	// build array
	std::vector<float> array;
	this->scene->buildArray(array);
	printf("build array complete\n");

	bindMesh(array);
	// printTreeBuffer(meshTextureBuffers[0], array.size());

	meshKDTree t;
	t.build(array);
	printf("kd tree node size: %d\n", t.nodes.size());
	printf("kd tree root node: %d\n", t.rootIndex);
	rootIndex = t.rootIndex;
	std::vector<float> kdtreeArray;
	t.buildArray(kdtreeArray);
	printf("build kd tree array complete\n");

	bindKDTree(kdtreeArray);
	// printTreeBuffer(treeTextureBuffers[0], kdtreeArray.size());
}

// bind scene meshes into gl texture buffer
void MyGLCanvas::bindPLY(glm::mat4 mat) {
	std::vector<float> array;
	this->myObjectPLY->buildArray(array, mat);
	printf("build array complete\n");

	bindMesh(array);
	// printTreeBuffer(meshTextureBuffers[0], array.size());

	meshKDTree t;
	t.build(array);
	printf("kd tree node size: %d\n", t.nodes.size());
	printf("kd tree root node: %d\n", t.rootIndex);
	rootIndex = t.rootIndex;
	std::vector<float> kdtreeArray;
	t.buildArray(kdtreeArray);
	printf("build kd tree array complete\n");

	bindKDTree(kdtreeArray);
	// printTreeBuffer(treeTextureBuffers[0], kdtreeArray.size());
}

void MyGLCanvas::initializeVertexBuffer() {
	// release
    if (vao) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    if (vbo) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }

    pixelIndices.clear();
    for (int j = 0; j < h(); j++) {
        for (int i = 0; i < w(); i++) {
            pixelIndices.push_back(float(i));
            pixelIndices.push_back(float(j));
        }
    }

    // VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // VBO
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // upload VBO
    glBufferData(GL_ARRAY_BUFFER, pixelIndices.size() * sizeof(float), pixelIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

	// release
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void MyGLCanvas::loadPLY(std::string filename) {
	delete myObjectPLY;
	myObjectPLY = new ply(filename);
	bindPLY(glm::mat4(1.0f));
	camera->reset();
	camera->setViewAngle(60.0f);
	updateCamera(w(), h());
	camera->orientLookAt(glm::vec3(0.0f, 1.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	printf("load ply complete\n");
}

void MyGLCanvas::loadNoise(std::string filename) {
	printf("loading noise file\n");
	myTextureManager->deleteTexture("noiseTex");
	myTextureManager->loadTexture3D("noiseTex", filename);
}

void MyGLCanvas::loadPlane() {
	delete myObjectPLY;
	char cwd[PATH_MAX];
	getcwd(cwd, sizeof(cwd));
	std::string pwd(cwd);
	std::cout << pwd + "/data/ply/airplane.ply" << endl;
	myObjectPLY = new ply(pwd + "/data/ply/airplane.ply");
	glm::mat4 mat(1.0f);
	mat = glm::rotate(mat, TO_RADIANS(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	mat = glm::rotate(mat, TO_RADIANS(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	bindPLY(glm::mat4(mat));
	camera->reset();
	camera->setViewAngle(60.0f);
	updateCamera(w(), h());
	camera->orientLookAt(glm::vec3(0.0f, 1.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	printf("load ply complete\n");
}

void MyGLCanvas::initializeFBO(int width, int height) {
    // use GL_TEXTURE1 gen color buffer
    glActiveTexture(GL_TEXTURE1);
    glGenTextures(1, &colorTexID);
    glBindTexture(GL_TEXTURE_2D, colorTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    // use GL_TEXTURE2 gen distance buffer
    glActiveTexture(GL_TEXTURE2);
    glGenTextures(1, &distanceTexID);
    glBindTexture(GL_TEXTURE_2D, distanceTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    // gen and bind FBO
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // attach texture to FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexID, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, distanceTexID, 0);

    GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBuffers);

    // check
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("Framebuffer not complete!\n");
    }

    // release
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // release
    glActiveTexture(GL_TEXTURE0);
}

void MyGLCanvas::resizeFBO(int width, int height) {
    // GL_TEXTURE1 for colorTexID
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, colorTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);

    // GL_TEXTURE2 for distanceTexID
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, distanceTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, nullptr);

    // release
    glBindTexture(GL_TEXTURE_2D, 0);

    // release
    glActiveTexture(GL_TEXTURE0);
}

void MyGLCanvas::readFBOData(int width, int height) {
    // 确保绑定了 FBO
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // 创建缓冲区存储数据
    std::vector<float> colorBuffer(width * height * 4); // RGBA，每像素4个值
    std::vector<float> distanceBuffer(width * height);  // 距离值，每像素1个值

    // 从 COLOR_ATTACHMENT0 读取颜色数据
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, colorBuffer.data());

    // 从 COLOR_ATTACHMENT1 读取距离数据
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    glReadPixels(0, 0, width, height, GL_RED, GL_FLOAT, distanceBuffer.data());

    // 找到前10个大于0的距离值并打印对应的颜色值
    std::cout << "Distance > 0 and corresponding Color:" << std::endl;
    int nonZeroCount = 0;
    for (int i = 0; i < width * height && nonZeroCount < 10; ++i) {
        if (distanceBuffer[i] > 0.0f) { // 条件：distance > 0
            int idx = i * 4; // 计算对应的 colorBuffer 索引
            std::cout << "Pixel " << i << ": Distance=" << distanceBuffer[i]
                      << ", Color(R,G,B,A)=(" << colorBuffer[idx] << ", "
                      << colorBuffer[idx + 1] << ", "
                      << colorBuffer[idx + 2] << ", "
                      << colorBuffer[idx + 3] << ")" << std::endl;
            ++nonZeroCount;
        }
    }

    // 恢复默认帧缓冲
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
