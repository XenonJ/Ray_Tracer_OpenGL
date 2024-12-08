#ifndef CYLINDER_H
#define CYLINDER_H

#include "Shape.h"

class Cylinder : public Shape {
public:
	Cylinder() { calculate(); };
	~Cylinder() {};

	OBJ_TYPE getType() {
		return SHAPE_CYLINDER;
	}
	
	std::vector<Vertex> vertices;
	std::vector<Face>   Faces;

	void draw();

	void drawNormal();

	void calculate();

	void drawTriangleMeshFromFaces();

	static float intersect(glm::vec4 ray, glm::vec4 eye);
	static glm::vec3 getNormal(glm::vec4 ray, glm::vec4 eye,float t);
	static glm::vec2 getXYCoordinate(glm::vec4 ray, glm::vec4 eye,float t);

private:
};

#endif