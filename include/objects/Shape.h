#ifndef SHAPE_H
#define SHAPE_H

#include <FL/gl.h>
#include <FL/glu.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <vector>
#include "Mesh.h"
#include "gfxDefs.h"

#define PI glm::pi<float>() // PI is now a constant for 3.14159....
#define MIN(x, y) x < y ? x : y

enum OBJ_TYPE {
    SHAPE_CUBE = 0,
    SHAPE_CYLINDER = 1,
    SHAPE_CONE = 2,
    SHAPE_SPHERE = 3,
    SHAPE_SPECIAL1 = 4,
    SHAPE_SPECIAL2 = 5,
    SHAPE_SPECIAL3 = 6,
    SHAPE_MESH = 7,
    SHAPE_SCENE = 8
};

class Shape {
public:
    static int m_segmentsX;
    static int m_segmentsY;

    Shape() { };
    virtual ~Shape() { for (auto g : graphs) { delete g; } };

    static void setSegments(int segX, int segY) {
        Shape::m_segmentsX = segX;
        Shape::m_segmentsY = segY;
    }

    void clearGraphs() {
        for (auto g : this->graphs) {
            delete g;
        }
        this->graphs.clear();
    }

    virtual OBJ_TYPE getType() = 0;
    virtual void     draw() {};
    virtual void     drawNormal() {};
    virtual void     calculate() {};
    virtual void     drawTriangleMeshFromFaces() {};
    virtual void     drawNormalsFromFaces() {};

    std::vector<Mesh*> graphs;

protected:

    void normalizeNormal(float x, float y, float z) { normalizeNormal(glm::vec3(x, y, z)); };

    void normalizeNormal(glm::vec3 v) {
        // glm::vec3 tmpV = glm::normalize(v);
        glNormal3f(v.x, v.y, v.z);
    };
};

#endif