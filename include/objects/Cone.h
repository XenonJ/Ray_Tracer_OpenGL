#ifndef CONE_H
#define CONE_H

#include "Shape.h"

class Cone : public Shape {
public:
	Cone() { calculate(); };
	~Cone() {};

	OBJ_TYPE getType() {
		return SHAPE_CONE;
	}

	void draw();

	void drawNormal();

	void calculate();

	void drawTriangleMeshFromFaces();

	void drawNormalsFromFaces();

	static float intersect(glm::vec4 ray, glm::vec4 eye);
	static glm::vec3 getNormal(glm::vec4 ray, glm::vec4 eye, float t);
	static glm::vec2 getXYCoordinate(glm::vec4 ray, glm::vec4 eye,float t);
private:
};

#endif