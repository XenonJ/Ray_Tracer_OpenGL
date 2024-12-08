#ifndef SPECIAL1_H
#define SPECIAL1_H

#include "Shape.h"

const float Radius = 0.5f;
const float radius = 0.25f;

class Torus : public Shape {
public:
	Torus() { calculate(); };
	~Torus() {};

	OBJ_TYPE getType() {
		return SHAPE_SPECIAL1;
	}

	void draw();

	void drawNormal();

	void calculate();

	void drawTriangleMeshFromFaces();

	void drawNormalsFromFaces();

	static float intersect(glm::vec4 ray, glm::vec4 eye);
	static glm::vec3 getNormal(glm::vec4 ray, glm::vec4 eye, float t);


private:
};

#endif