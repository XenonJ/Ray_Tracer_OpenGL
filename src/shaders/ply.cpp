/*  =================== File Information =================
File Name: ply.cpp
Description:
Author: (You)

Purpose:
Examples:
===================================================== */
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <cstdlib>
#include "shaders/ply.h"
#include "geometry.h"
#include <math.h>


using namespace std;

/*  ===============================================
Desc: Default constructor for a ply object
Precondition:
Postcondition:
=============================================== */
ply::ply() {
	vertexList = NULL;
	faceList = NULL;
	vertexArray = NULL;
    indiciesArray = NULL;
    normalsArray = NULL;
	properties = 0;
	faceCount = 0;
	vertexCount = 0;
	vao = -1;
	vertexVBO_id = -1;
	indicesVBO_id = -1;
	normalVBO_id = -1;
	vertexArray = NULL;
	indiciesArray = NULL;
	normalsArray = NULL;
}

/*  ===============================================
Desc: constructor for a ply object with a default path
Precondition:
Postcondition:
=============================================== */
ply::ply(string filePath) {
	vertexList = NULL;
	faceList = NULL;
	vertexArray = NULL;
	indiciesArray = NULL;
	normalsArray = NULL;
	faceCount = 0;
	vertexCount = 0;
	properties = 0;
	vao = -1;
	vertexVBO_id = -1;
	indicesVBO_id = -1;
	normalVBO_id = -1;
	vertexList = NULL;
	faceList = NULL;
	vertexArray = NULL;
	reload(filePath);
}


/*  ===============================================
Desc: Destructor for a ply object
Precondition: Memory has been already allocated
Postcondition:
=============================================== */
ply::~ply() {
	reset();
}

void ply::reset() {
	// Delete the allocated arrays
	if (vertexList != NULL)
		delete[] vertexList;

	for (int i = 0; i < faceCount; i++) {
		delete[] faceList[i].vertexList;
	}

	if (faceList != NULL)
		delete[] faceList;
	// Set pointers to NULL
	vertexList = NULL;
	faceList = NULL;


	if (vertexArray != NULL) {
		delete[] vertexArray;
		vertexArray = NULL;
	}

	if (indiciesArray != NULL) {
		delete[] indiciesArray;
		indiciesArray = NULL;
	}

	if (normalsArray != NULL) {
		delete[] normalsArray;
		normalsArray = NULL;
	}

	// Destroy our vertex buffer object
	if (vertexVBO_id != -1)
		glDeleteBuffers(1, &vertexVBO_id);
	if (indicesVBO_id != -1)
		glDeleteBuffers(1, &indicesVBO_id);
	if (normalVBO_id != -1)
		glDeleteBuffers(1, &normalVBO_id);

	vao = -1;
	vertexVBO_id = -1;
	indicesVBO_id = -1;
	normalVBO_id = -1;
}


/*  ===============================================
Desc: reloads the geometry for a 3D object
Precondition:
Postcondition:
=============================================== */
void ply::reload(string _filePath) {
	filePath = _filePath;
	reset();

	// Call our function again to load new vertex and face information.
	loadGeometry();
}

/*  ===============================================
Desc: You get to implement this
Precondition:
Postcondition:
=============================================== */
void ply::loadGeometry() {

	/* You will implement this section of code
	1. Parse the header
	2.) Update any private or helper variables in the ply.h private section
	3.) allocate memory for the vertexList
	3a.) Populate vertices
	4.) allocate memory for the faceList
	4a.) Populate faceList
	*/


	ifstream myfile(filePath.c_str()); // load the file
	if (myfile.is_open()) { // if the file is accessable
		properties = -2; // set the properties because there are extras labeled

		string line;
		char* token_pointer;
		char* lineCopy = new char[256];
		int count;
		bool reading_header = true;
		// loop for reading the header 
		while (reading_header && getline(myfile, line)) {

			// get the first token in the line, this will determine which
			// action to take. 
			strcpy(lineCopy, line.c_str());
			token_pointer = strtok(lineCopy, " \r");
			// case when the element label is spotted:
			if (strcmp(token_pointer, "element") == 0) {
				token_pointer = strtok(NULL, " \r");

				// When the vertex token is spotted read in the next token
				// and use it to set the vertexCount and initialize vertexList
				if (strcmp(token_pointer, "vertex") == 0) {
					token_pointer = strtok(NULL, "  \r");
					vertexCount = atoi(token_pointer);
					vertexList = new vertex[vertexCount];
				}

				// When the face label is spotted read in the next token and 
				// use it to set the faceCount and initialize faceList.
				if (strcmp(token_pointer, "face") == 0) {
					token_pointer = strtok(NULL, "  \r");
					faceCount = atoi(token_pointer);
					faceList = new face[faceCount];
				}
			}
			// if property label increment the number of properties.
			if (strcmp(token_pointer, "property") == 0) { properties++; }
			// if end_header break the header loop and move to reading vertices.
			if (strcmp(token_pointer, "end_header") == 0) { reading_header = false; }
		}

		// Read in exactly vertexCount number of lines after reading the header
		// and set the appropriate vertex in the vertexList.
		for (int i = 0; i < vertexCount; i++) {

			getline(myfile, line);
			strcpy(lineCopy, line.c_str());

			// by convention the first three are x, y, z and we ignore the rest
			if (properties >= 0) {
				vertexList[i].x = atof(strtok(lineCopy, " \r"));
			}
			if (properties >= 1) {
				vertexList[i].y = atof(strtok(NULL, " \r"));
			}
			if (properties >= 2) {
				vertexList[i].z = atof(strtok(NULL, " \r"));
			}
		}

		// Read in the faces (exactly faceCount number of lines) and set the 
		// appropriate face in the faceList
		for (int i = 0; i < faceCount; i++) {

			getline(myfile, line);

			strcpy(lineCopy, line.c_str());
			count = atoi(strtok(lineCopy, " \r"));
			faceList[i].vertexCount = count; // number of vertices stored 
			faceList[i].vertexList = new int[count]; // initialize the vertices

			// set the vertices from the input, reading only the number of 
			// vertices that are specified
			for (int j = 0; j < count; j++) {
				faceList[i].vertexList[j] = atoi(strtok(NULL, " \r"));
			}
		}
		delete(lineCopy);
		myfile.close();
		scaleAndCenter();
		cout << "completed loading: " << filePath.c_str() << "\n";
	}
	// if the path is invalid, report then exit.
	else {
		cout << "cannot open file " << filePath.c_str() << "\n";
	}
};

/*  ===============================================
Desc: Moves all the geometry so that the object is centered at 0, 0, 0 and scaled to be between 0.5 and -0.5
Precondition: after all the vetices and faces have been loaded in
Postcondition:
=============================================== */
void ply::scaleAndCenter() {
	float avrg_x = 0.0;
	float avrg_y = 0.0;
	float avrg_z = 0.0;
	float max = 0.0;
	int   i;

	// loop through each vertex in the given image
	for (i = 0; i < vertexCount; i++) {

		// obtain the total for each property of the vertex
		avrg_x += vertexList[i].x;
		avrg_y += vertexList[i].y;
		avrg_z += vertexList[i].z;
	}

	// compute the average for each property
	avrg_x = avrg_x / vertexCount;
	avrg_y = avrg_y / vertexCount;
	avrg_z = avrg_z / vertexCount;

	// center each vertex
	for (i = 0; i < vertexCount; i++) {
		vertexList[i].x = (vertexList[i].x - avrg_x);
		vertexList[i].y = (vertexList[i].y - avrg_y);
		vertexList[i].z = (vertexList[i].z - avrg_z);
	}

	// find the range of the vertices
	for (i = 0; i < vertexCount; i++) {
		// obtain the max dimension to find the furthest point from 0,0
		if (max < fabs(vertexList[i].x)) max = fabs(vertexList[i].x);
		if (max < fabs(vertexList[i].y)) max = fabs(vertexList[i].y);
		if (max < fabs(vertexList[i].z)) max = fabs(vertexList[i].z);
	}

	// max is doubled so that the range we get is from 0.5 to -0.5
	max *= 2.0f;

	// scale each vertex to fit within the bounds
	for (i = 0; i < vertexCount; i++) {
		vertexList[i].x = vertexList[i].x / max;
		vertexList[i].y = vertexList[i].y / max;
		vertexList[i].z = vertexList[i].z / max;
	}
}

/*  ===============================================
Desc: Draws a filled 3D object
Precondition:
Postcondition:
Error Condition: If we haven't allocated memory for our
faceList or vertexList then do not attempt to render.
=============================================== */
// void ply::render(){
// 	if (vertexList == NULL || faceList == NULL){
// 		return;
// 	}

// 	glPushMatrix();
// 	// For each of our faces
// 	for (int i = 0; i < faceCount; i++) {

// 		// All of our faces are actually triangles for PLY files

// 		glBegin(GL_TRIANGLES);

// 		// Get the vertex list from the face list.  Each element is not the 
// 		// vertex itself but the index of the vertex within the vertexList
// 		// For this project we don't care about the intesity, confidence, 
// 		// nx, ny or nz.

// 		int index0 = faceList[i].vertexList[0];
// 		int index1 = faceList[i].vertexList[1];
// 		int index2 = faceList[i].vertexList[2];

// 		// using the setNormal function from below we normalize the vectors
// 		setNormal(vertexList[index0].x, vertexList[index0].y, vertexList[index0].z,
// 			vertexList[index1].x, vertexList[index1].y, vertexList[index1].z,
// 			vertexList[index2].x, vertexList[index2].y, vertexList[index2].z);


// 		for (int j = 0; j < faceList[i].vertexCount; j++){
// 			// Get each vertices x,y,z and draw them
// 			int index = faceList[i].vertexList[j];
// 			glVertex3f(vertexList[index].x, vertexList[index].y, vertexList[index].z);
// 		}
// 		glEnd();
// 	}
// 	glPopMatrix();
// }

/*  ===============================================
Desc: Prints some statistics about the file you have read in
This is useful for debugging information to see if we parse our file correctly.

Precondition:
Postcondition:
=============================================== */
void ply::printAttributes() {
	cout << "==== ply Mesh Attributes=====" << endl;
	cout << "vertex count:" << vertexCount << endl;
	cout << "face count:" << faceCount << endl;
	cout << "properties:" << properties << endl;
}

/*  ===============================================
Desc: Iterate through our array and print out each vertex.

Precondition:
Postcondition:
=============================================== */
void ply::printVertexList() {
	if (vertexList == NULL) {
		return;
	}
	else {
		for (int i = 0; i < vertexCount; i++) {
			cout << vertexList[i].x << "," << vertexList[i].y << "," << vertexList[i].z << endl;
		}
	}
}

/*  ===============================================
Desc: Iterate through our array and print out each face.

Precondition:
Postcondition:
=============================================== */
void ply::printFaceList() {
	if (faceList == NULL) {
		return;
	}
	else {
		// For each of our faces
		for (int i = 0; i < faceCount; i++) {
			// Get the vertices that make up each face from the face list
			for (int j = 0; j < faceList[i].vertexCount; j++) {
				// Print out the vertex
				int index = faceList[i].vertexList[j];
				cout << vertexList[index].x << "," << vertexList[index].y << "," << vertexList[index].z << endl;
			}
		}
	}
}

void ply::setNormal(float x1, float y1, float z1,
	float x2, float y2, float z2,
	float x3, float y3, float z3) {

	float v1x, v1y, v1z;
	float v2x, v2y, v2z;
	float cx, cy, cz;

	//find vector between x2 and x1
	v1x = x1 - x2;
	v1y = y1 - y2;
	v1z = z1 - z2;

	//find vector between x3 and x2
	v2x = x2 - x3;
	v2y = y2 - y3;
	v2z = z2 - z3;

	//cross product v1xv2

	cx = v1y * v2z - v1z * v2y;
	cy = v1z * v2x - v1x * v2z;
	cz = v1x * v2y - v1y * v2x;

	//normalize

	float length = sqrt(cx * cx + cy * cy + cz * cz);
	cx = cx / length;
	cy = cy / length;
	cz = cz / length;

	//glNormal3f(cx, cy, cz);
}

/*  ===============================================
Desc:   Finds the normal to three vertices(1 face making up a triangle)
Precondition:
Postcondition:  returns the normals as outputx, outputy, and outputz
=============================================== */
void ply::computeNormal(float x1, float y1, float z1,
	float x2, float y2, float z2,
	float x3, float y3, float z3,
	float* outputx, float* outputy, float* outputz)
{

	float v1x, v1y, v1z;
	float v2x, v2y, v2z;
	float cx, cy, cz;

	//find vector between x2 and x1
	v1x = x1 - x2;
	v1y = y1 - y2;
	v1z = z1 - z2;

	//find vector between x3 and x2
	v2x = x2 - x3;
	v2y = y2 - y3;
	v2z = z2 - z3;

	//cross product v1xv2

	cx = v1y * v2z - v1z * v2y;
	cy = v1z * v2x - v1x * v2z;
	cz = v1x * v2y - v1y * v2x;

	//normalize

	float length = sqrt(cx * cx + cy * cy + cz * cz);
	cx = cx / length;
	cy = cy / length;
	cz = cz / length;

	*outputx = cx;
	*outputy = cy;
	*outputz = cz;

	//glNormal3f(cx, cy, cz);
}


/*  ===============================================
  Desc: Simple function that builds all of the arrays to be used
		in vertex buffer objects.


  Precondition:
  Postcondition:
=============================================== */
void ply::buildArrays() {
	// allocate memory for our arrays
	vertexArray = new GLfloat[vertexCount * 3];
	if (vertexArray == NULL) {
		cout << "Ran out of memory(vertexArray)!" << endl;
		return;
	}

	indiciesArray = new GLuint[faceCount * 3];
	if (indiciesArray == NULL) {
		cout << "Ran out of memory(indiciesArray)!" << endl;
		return;
	}

	normalsArray = new GLfloat[vertexCount * 3];
	if (normalsArray == NULL) {
		cout << "Ran out of memory(normalsArray)!" << endl;
		return;
	}
	for (int i = 0; i < vertexCount * 3; i++) {
		normalsArray[i] = 0.0;
	}

	int* numNormals = new int[vertexCount];
	if (numNormals == NULL) {
		cout << "Ran out of memory(normal Counter)!" << endl;
		return;
	}
	for (int i = 0; i < vertexCount; i++) {
		numNormals[i] = 0;
	}


	// Compress all of the vertices into a simple array
	if (vertexList == NULL) {
		return;
	}
	else {
		for (int i = 0; i < vertexCount; i++) {
			vertexArray[i * 3 + 0] = vertexList[i].x;
			vertexArray[i * 3 + 1] = vertexList[i].y;
			vertexArray[i * 3 + 2] = vertexList[i].z;
		}
	}

	// Compress everything into one array of indices
	unsigned int k = 0;
	if (faceList == NULL) {
		return;
	}
	else {
		// For each of our faces
		for (int i = 0; i < faceCount; i++) {
			// Get the vertices that make up each face from the face list
			int index0 = faceList[i].vertexList[0];
			int index1 = faceList[i].vertexList[1];
			int index2 = faceList[i].vertexList[2];

			indiciesArray[k] = index0;
			indiciesArray[k + 1] = index1;
			indiciesArray[k + 2] = index2;

			k += 3;
		}
	}

	//this part is for building the vertex normals -- it looks complicated because 
	// we are computing the "smooth" normals of a vertex, which is defined as the "average"
	// of all the normals at a vertex. 
	//That is, let's say that a vertex is at the intersection of 5 triangles, each triangle
	// will have its own normal (computed using the cross product).
	//To determine the "smooth" normal at that vertex, we add up all 5 triangle normals and then
	// divide by 5.
	//In order to do this, there are two steps:
	//  1. we sum up all the normals. 
	//  2. While we are doing that, we need to keep a counter for the number of normals at a vertex
	for (int i = 0; i < faceCount * 3; i = i + 3) {
		int index0 = indiciesArray[i];
		int index1 = indiciesArray[i + 1];
		int index2 = indiciesArray[i + 2];

		float outputx, outputy, outputz;
		// using the setNormal function from below we normalize the vectors
		computeNormal(
			vertexArray[index0 * 3 + 0], vertexArray[index0 * 3 + 1], vertexArray[index0 * 3 + 2],
			vertexArray[index1 * 3 + 0], vertexArray[index1 * 3 + 1], vertexArray[index1 * 3 + 2],
			vertexArray[index2 * 3 + 0], vertexArray[index2 * 3 + 1], vertexArray[index2 * 3 + 2],
			&outputx, &outputy, &outputz);

		numNormals[index0]++;
		numNormals[index1]++;
		numNormals[index2]++;

		normalsArray[index0 * 3 + 0] += outputx;
		normalsArray[index0 * 3 + 1] += outputy;
		normalsArray[index0 * 3 + 2] += outputz;

		normalsArray[index1 * 3 + 0] += outputx;
		normalsArray[index1 * 3 + 1] += outputy;
		normalsArray[index1 * 3 + 2] += outputz;

		normalsArray[index2 * 3 + 0] += outputx;
		normalsArray[index2 * 3 + 1] += outputy;
		normalsArray[index2 * 3 + 2] += outputz;
	}
	for (int i = 0; i < vertexCount; i++) {
		normalsArray[i * 3 + 0] = normalsArray[i * 3 + 0] / (float)(numNormals[i]);
		normalsArray[i * 3 + 1] = normalsArray[i * 3 + 1] / (float)(numNormals[i]);
		normalsArray[i * 3 + 2] = normalsArray[i * 3 + 2] / (float)(numNormals[i]);
	}


	delete[] numNormals;
}

void ply::bindVBO(unsigned int programID) {

	// Use a Vertex Array Object -- think of this as a single ID that sums up all the following VBOs
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Generate a buffer that will be sent to the video memory
	// For each ply object, it will get its own vertex buffer object, but it
	// would be much more efficient to put everything into one Vertex Buffer Object and send it over
	// only once to video memory. 

	// Note: If this seg faults, then Glee or Glew (however OpenGL 2.0 extensions are mangaged)
	// has not yet been initialized.
	//tell openGL to generate a new VBO object
	glGenBuffers(1, &vertexVBO_id);
	// Once we know how many buffers to generate, then hook up the buffer to the vertexVBO_id.
	glBindBuffer(GL_ARRAY_BUFFER, vertexVBO_id);
	// Now we finally copy data into the buffer object
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertexCount * 3, vertexArray, GL_STATIC_DRAW);


	//tell openGL to generate a new VBO object
	glGenBuffers(1, &indicesVBO_id);
	// Transfer the data from indices to a VBO indicesVBO_id
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesVBO_id);
	// Copy data into the buffer object. Note the keyword difference here -- GL_ELEMENT_ARRAY_BUFFER
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * faceCount * 3, indiciesArray, GL_STATIC_DRAW);
	// Get the location of the attributes that enters in the vertex shader
	GLint position_attribute = glGetAttribLocation(programID, "myPosition");
	// Specify how the data for position can be accessed
	glVertexAttribPointer(position_attribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	// Enable the attribute
	glEnableVertexAttribArray(position_attribute);


	//Repeat the process as building a vertexVBO, but this time for the normals
	glGenBuffers(1, &normalVBO_id);
	// bind the newly generated buffer to the normalVBO_id.
	glBindBuffer(GL_ARRAY_BUFFER, normalVBO_id);
	// Now we finally copy data into the buffer object
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertexCount * 3, normalsArray, GL_STATIC_DRAW);
	// Get the location of the attributes that enters in the vertex shader
	GLint normal_attribute = glGetAttribLocation(programID, "myNormal");
	// Specify how the data for position can be accessed
	glVertexAttribPointer(normal_attribute, 3, GL_FLOAT, GL_TRUE, 0, 0);
	// Enable the attribute
	glEnableVertexAttribArray(normal_attribute);

	cout << "Created vbo successfully" << endl;
}

void ply::renderVBO(unsigned int shaderProgramID) {
	//bindVBO(shaderProgramID);
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, faceCount * 3, GL_UNSIGNED_INT, 0);



	//************The following code is for using VBO but not shaders ****************//
	//glDrawArrays(GL_TRIANGLES, 0, faceCount);

	//glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eab);

	//glEnableClientState(GL_VERTEX_ARRAY);
	//glVertexPointer(3, GL_FLOAT, 0, (char*)NULL + 0);
	//glDrawElements(GL_TRIANGLES, faceCount*3, GL_UNSIGNED_INT, 0);

	//glDisableClientState(GL_VERTEX_ARRAY);



	//************The following code is for using the old fixed pipeline (so called immediate mode) ****************//
	//int i;

	//glBegin(GL_TRIANGLES);

	//for (i = 0; i < faceCount*3; i=i+3) {
	//	int index0 = indicies_vao[i];
	//	int index1 = indicies_vao[i+1];
	//	int index2 = indicies_vao[i+2];

	//	glNormal3f(normals_vao[index0 * 3], normals_vao[index0 * 3 + 1], normals_vao[index0 * 3 + 2]);
	//	glVertex3f(vertex_vao[index0*3], vertex_vao[index0*3+1], vertex_vao[index0*3+2]);

	//	glNormal3f(normals_vao[index1 * 3], normals_vao[index1 * 3 + 1], normals_vao[index1 * 3 + 2]);
	//	glVertex3f(vertex_vao[index1 * 3], vertex_vao[index1 * 3 + 1], vertex_vao[index1 * 3 + 2]);

	//	glNormal3f(normals_vao[index2 * 3], normals_vao[index2 * 3 + 1], normals_vao[index2 * 3 + 2]);
	//	glVertex3f(vertex_vao[index2 * 3], vertex_vao[index2 * 3 + 1], vertex_vao[index2 * 3 + 2]);
	//}

	//glEnd();
}

/*
    array structure
     1 -  9: vertices
    10 - 12: face normal
    13 - 15: rgb
    16 - 18: mesh type
*/
void ply::buildArray(std::vector<float>& array) {
	for (int i = 0; i < faceCount; i++) {
		// 1 - 9
		// Get the vertices that make up each face from the face list, assuming all faces have 3 vertices
		for (int j = 0; j < 3; j++) {
			// Print out the vertex
			int index = faceList[i].vertexList[j];
			// cout << vertexList[index].x << "," << vertexList[index].y << "," << vertexList[index].z << endl;
			array.push_back(vertexList[index].x);
			array.push_back(vertexList[index].y);
			array.push_back(vertexList[index].z);
		}
		// 10 - 12
		float nx, ny, nz;
		computeNormal(
			vertexList[faceList[i].vertexList[0]].x, vertexList[faceList[i].vertexList[0]].y, vertexList[faceList[i].vertexList[0]].z,
			vertexList[faceList[i].vertexList[1]].x, vertexList[faceList[i].vertexList[1]].y, vertexList[faceList[i].vertexList[1]].z,
			vertexList[faceList[i].vertexList[2]].x, vertexList[faceList[i].vertexList[2]].y, vertexList[faceList[i].vertexList[2]].z,
			&nx, &ny, &nz
		);
		array.push_back(nx);
		array.push_back(ny);
		array.push_back(nz);
		// 13 - 15
		array.push_back(1.0f);
		array.push_back(1.0f);
		array.push_back(1.0f);
		// 16 - 18
		array.push_back(1.0f);
		array.push_back(0.0f);
		array.push_back(0.0f);
	}
}
