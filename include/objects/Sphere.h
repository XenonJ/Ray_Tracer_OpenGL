#ifndef SPHERE_H
#define SPHERE_H

#include "Shape.h"

class Sphere : public Shape {
public:
	Sphere() { calculate(); };
	~Sphere() {};

	OBJ_TYPE getType() {
		return SHAPE_SPHERE;
	}

	void draw();

	void drawNormal();

	void calculate();

	void drawTriangleMeshFromFaces();

	void drawNormalsFromFaces();

	static float intersect(glm::vec4 rayV, glm::vec4 eyePointP);
	static glm::vec3 getNormal(glm::vec4 rayV, glm::vec4 eyePointP,float t);
	static glm::vec2 getXYCoordinate(glm::vec4 ray, glm::vec4 eye,float t);

private:
};

#endif