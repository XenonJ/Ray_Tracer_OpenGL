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
}

MyGLCanvas::~MyGLCanvas() {
	delete myTextureManager;
	delete myShaderManager;
	delete myObjectPLY;
}

void MyGLCanvas::initShaders() {
	printf("init shaders\n");
	myTextureManager->loadTexture3D("noiseTex", "./data/ppm/tiled_worley_noise.ppm");

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

	// pass camera data
	glUniform3fv(eyeLoc, 1, glm::value_ptr(camera->getEyePoint()));
	// printf("eyepoint: %f %f %f\n", 
	// 	camera->getEyePoint().x, camera->getEyePoint().y, camera->getEyePoint().z
	// );
	glUniform3fv(lookVecLoc, 1, glm::value_ptr(camera->getLookVector()));
	glUniform3fv(upVecLoc, 1, glm::value_ptr(camera->getUpVector()));
	glUniform1f(viewAngleLoc, camera->getViewAngle());
	glUniform1f(nearLoc, camera->getNearPlane());
	glUniform1f(widthLoc, camera->getScreenWidth());
	glUniform1f(heightLoc, camera->getScreenHeight());
	glUniform3fv(lightPosLoc, 1, glm::value_ptr(glm::vec3(3.0f)));	// default light

	frameCounter++;
	if (frameCounter == INT_MAX)
	{
		frameCounter = 0;
	}
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
		// printf("num mesh buffers: %d, max triangles per buffer: %d\n", this->meshTextureBuffers.size(), int(maxTrianglesPerBuffer));
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
		// printf("num mesh buffers: %d, max triangles per buffer: %d\n", this->meshTextureBuffers.size(), int(maxTrianglesPerBuffer));
		GLint treeSizeLoc = glGetUniformLocation(myShaderManager->getShaderProgram("objectShaders")->programID, "treeSize");
		glUniform1i(treeSizeLoc, treeSize);

		// pass light
		SceneLightData lightData;
		if (parser && parser->getLightData(0, lightData)) {
			glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightData.pos));
		}
		// printf("mesh size: %d\n", meshSize);
	}

    // draw pixels
    glDrawArrays(GL_POINTS, 0, w() * h());
	glClear(GL_DEPTH_BUFFER_BIT);

	// draw environment
	glUseProgram(myShaderManager->getShaderProgram("environmentShaders")->programID);
	 // bind vao
    glBindVertexArray(vao);
    // pass Uniform
    eyeLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "eyePosition");
    lookVecLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "lookVec");
    upVecLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "upVec");
    viewAngleLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "viewAngle");
    nearLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "nearPlane");
    widthLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "screenWidth");
    heightLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "screenHeight");
    lightPosLoc = glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "lightPos");
	// pass camera data
	glUniform3fv(eyeLoc, 1, glm::value_ptr(camera->getEyePoint()));
	// printf("eyepoint: %f %f %f\n", 
	// 	camera->getEyePoint().x, camera->getEyePoint().y, camera->getEyePoint().z
	// );
	glUniform3fv(lookVecLoc, 1, glm::value_ptr(camera->getLookVector()));
	glUniform3fv(upVecLoc, 1, glm::value_ptr(camera->getUpVector()));
	glUniform1f(viewAngleLoc, camera->getViewAngle());
	glUniform1f(nearLoc, camera->getNearPlane());
	glUniform1f(widthLoc, camera->getScreenWidth());
	glUniform1f(heightLoc, camera->getScreenHeight());
	glUniform3fv(lightPosLoc, 1, glm::value_ptr(glm::vec3(300.0f)));	// default light

	frameCounter++;
	if (frameCounter == INT_MAX)
	{
		frameCounter = 0;
	}
	glUniform1i(glGetUniformLocation(myShaderManager->getShaderProgram("environmentShaders")->programID, "frameCounter"), frameCounter);

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
void MyGLCanvas::bindPLY() {
	std::vector<float> array;
	this->myObjectPLY->buildArray(array);
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
	bindPLY();
	camera->reset();
	camera->setViewAngle(60.0f);
	updateCamera(w(), h());
	camera->orientLookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	printf("load ply complete\n");
}
