#include "objects/Cylinder.h"
#include "objects/Mesh.h"
#include <iostream>


float radius = 0.5f;

void Cylinder::drawTriangleMeshFromFaces() {
    // Get current mode
    GLint shadeModel;
    glGetIntegerv(GL_SHADE_MODEL, &shadeModel);

    // Draw side face
    glBegin(GL_TRIANGLES);

    for (Mesh* g : this->graphs) {
        for (Face* face : *g->getFaceIterator()) {
            Vertex* const* verts = face->getVertices();
            for (int i = 0; i < 3; i++)
            {
                Vertex* v = verts[i];
                if (shadeModel == GL_SMOOTH)
                {
                    normalizeNormal(v->getNormals());
                }
                else if (shadeModel == GL_FLAT)
                {
                    normalizeNormal(face->getFaceNormal());
                }

                glm::vec3 pos = v->getPos();
                glVertex3f(pos.x, pos.y, pos.z);
            }
        }
    }
    glEnd();

}

void Cylinder::draw() {
    drawTriangleMeshFromFaces();
}

void Cylinder::drawNormal() {
    glColor3f(1.0f, .0f, .0f);

    glBegin(GL_LINES);
    for (Mesh* g : this->graphs) {
        for (Vertex* v : *g->getVertexIterator()) {
            const glm::vec3& normal = v->getNormals();
            const glm::vec3& pos = (v->getPos());

            glVertex3f(pos.x, pos.y, pos.z);
            glVertex3f(pos.x + normal.x * .1f, pos.y + normal.y * .1f,
                pos.z + normal.z * .1f);
        }
    }
    glEnd();
}

void Cylinder::calculate() {
    int vcount = (m_segmentsY + 1) * m_segmentsX;
    int fcount = m_segmentsX * m_segmentsY * 2;
    //  Create a new graph to store the side shape
    Mesh* side = new Mesh(vcount, fcount);
    Mesh* topFace = new Mesh(m_segmentsX + 1, m_segmentsX + 1);
    Mesh* bottomFace = new Mesh(m_segmentsX + 1, m_segmentsX + 1);

    this->clearGraphs();

    float stepAngle = 360.0f / m_segmentsX;
    float stepY = 1.0f / m_segmentsY;

    this->vertices.clear();
    this->Faces.clear();


    for (int i = 0; i <= m_segmentsY; i++) {
        float y = i * stepY - radius;

        for (int j = 0; j < m_segmentsX; j++) {
            float angle = glm::radians(j * stepAngle);
            float x = radius * glm::cos(angle);
            float z = radius * glm::sin(angle);

            glm::vec3 position(x, y, z);
            // Calculate normal
            glm::vec3 normal = glm::vec3(x, 0.0f, z);

            // Build vertex with position and normal
            Vertex* v = new Vertex(position);

            v->setNormal(glm::vec3(x, 0.0f, z));
            side->addVertex(v);
            /* Add vertice on top and bottom to their graph */

            if (i == 0) // This point to the bottom
            {
                Vertex* vbottom = new Vertex(position);
                glm::vec3 bottomNormal = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));
                vbottom->setNormal(bottomNormal);
                bottomFace->addVertex(vbottom);
            }

            if (i == m_segmentsY) // This point to the top
            {
                Vertex* vtop = new Vertex(position);
                glm::vec3 topNormal = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f));
                vtop->setNormal(topNormal);
                topFace->addVertex(vtop);
            }
        }
    }

    // Get the vertices list for calculating the face 
    Vertex** verts = side->getVertices();
    // Add all side face to side shape
    for (int i = 0; i < m_segmentsY; ++i) {
        for (int j = 0; j < m_segmentsX; ++j) {
            int index1 = i * m_segmentsX + j;
            int index2 = (j + 1) % m_segmentsX + i * m_segmentsX;
            int index3 = (i + 1) * m_segmentsX + j;
            int index4 = (i + 1) * m_segmentsX + ((j + 1) % m_segmentsX);

            Face* f1 = new Face(verts[index1], verts[index3], verts[index4]);

            // verts[index1]->addFace(f1);
            // verts[index3]->addFace(f1);
            // verts[index4]->addFace(f1);

            Face* f2 = new Face(verts[index4], verts[index2], verts[index1]);

            // verts[index4]->addFace(f2);
            // verts[index2]->addFace(f2);
            // verts[index1]->addFace(f2);

            side->addFace(f1);
            side->addFace(f2);
        }
    }

    /* --------------------- Add the top and bottom center points to the graph --------------------*/
    glm::vec3 topCenter(0, 1.0f - radius, 0);
    glm::vec3 bottomCenter(0, -radius, 0);

    // int topCenterIndex = this->vertices.size();
    glm::vec3 topNormal = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f));
    Vertex* top = new Vertex(topCenter);
    top->setNormal(topNormal);
    topFace->addVertex(top);

    // int bottomCenterIndex = this->vertices.size();
    glm::vec3 bottomNormal = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));
    Vertex* bottom = new Vertex(bottomCenter);
    bottom->setNormal(bottomNormal);
    bottomFace->addVertex(bottom);



    // Calculate and add the top and bottom face

    Vertex** topVertices = topFace->getVertices();
    Vertex** bottomVertices = bottomFace->getVertices();

    for (int i = 0; i < m_segmentsX; i++)
    {
        int index1 = i;
        int index2 = (i + 1) % (m_segmentsX);
        Face* f1 = new Face(topVertices[m_segmentsX], topVertices[index2], topVertices[index1]);
        topFace->addFace(f1);
    }

    for (int j = 0; j < m_segmentsX; j++)
    {
        int index1 = j;
        int index2 = (j + 1) % (m_segmentsX);
        Face* f2 = new Face(bottomVertices[m_segmentsX], bottomVertices[index1], bottomVertices[index2]);
        bottomFace->addFace(f2);
    }

    // Push the graph to STL for managing
    this->graphs.push_back(side);
    this->graphs.push_back(topFace);
    this->graphs.push_back(bottomFace);

    // Print total size
    // int verticesSize = 0, facesSize = 0;
    // for (Mesh* g : this->graphs)
    // {
    //     verticesSize += g->getVertices().size();
    //     facesSize += g->getFaces().size();
    // }



    // std::cout << "Vertices count: " << verticesSize - 2 * m_segmentsX << std::endl;
    // std::cout << "Faces count: " << facesSize << std::endl;
}

float Cylinder::intersect(glm::vec4 ray, glm::vec4 eye) {
    float t = -1.0f;
    float A = ray.x * ray.x + ray.z * ray.z;
    float B = 2 * eye.x * ray.x + 2 * eye.z * ray.z;
    float C = eye.x * eye.x + eye.z * eye.z - 0.25f;
    float delta = B * B - 4 * A * C;
    if (delta > 0) {
        float t1 = (-B + glm::sqrt(delta)) / (2 * A);
        float t2 = (-B - glm::sqrt(delta)) / (2 * A);
        float tmpy1 = eye.y + t1 * ray.y;
        float tmpy2 = eye.y + t2 * ray.y;
        if (glm::abs(tmpy1) < 0.5f + EPSILON) {
            if (t1 > 0) {
                if (t < 0) t = t1;
                else t = MIN(t, t1);
            }
        }
        if (glm::abs(tmpy2) < 0.5f + EPSILON) {
            if (t2 > 0) {
                if (t < 0) t = t2;
                else t = MIN(t, t2);
            }
        }
    }
    // CAP and BOTTOM
    float tmpt, tmpx, tmpy, tmpz;
    tmpt = (0.5f - eye.y) / ray.y;
    tmpx = eye.x + tmpt * ray.x;
    tmpy = eye.y + tmpt * ray.y;
    tmpz = eye.z + tmpt * ray.z;
    if (tmpx * tmpx + tmpz * tmpz < 0.5 * 0.5 + EPSILON) {
        if (t < 0) {
            t = tmpt;
        }
        else {
            t = MIN(t, tmpt);
        }
    }
    tmpt = (-0.5f - eye.y) / ray.y;
    tmpx = eye.x + tmpt * ray.x;
    tmpy = eye.y + tmpt * ray.y;
    tmpz = eye.z + tmpt * ray.z;
    if (tmpx * tmpx + tmpz * tmpz < 0.5 * 0.5 + EPSILON) {
        if (t < 0) {
            t = tmpt;
        }
        else {
            t = MIN(t, tmpt);
        }
    }
    return t;
}

glm::vec2 Cylinder::getXYCoordinate(glm::vec4 ray, glm::vec4 eye,float t) {
    float x = eye.x + t * ray.x;
    float y = eye.y + t * ray.y;
    float z = eye.z + t * ray.z;

    float theta = PI + atan2(z, x); // Dont know why but it works
    float u = 0.0;
    float v = -0.5-y;;
    
    if (theta < 0) {
        u = - (theta / (2*PI));
    } else {
        u = 1 - (theta / (2*PI));
    }
    
    //CAP
    if (glm::abs(y - 0.5) < EPSILON) {
        return glm::vec2(x+0.5, 0.5-z);
    }
    //BOTTOM
    else if (glm::abs(y - -0.5) < EPSILON) { // check
        return glm::vec2(x+0.5, -0.5+z);
    }
    //BODY
    else {
        return glm::vec2(u, v);
    }
}

glm::vec3 Cylinder::getNormal(glm::vec4 ray, glm::vec4 eye,float t) {
    float x = eye.x + t * ray.x;
    float y = eye.y + t * ray.y;
    float z = eye.z + t * ray.z;
    //CAP
    if (glm::abs(y - 0.5) < EPSILON) {
        return glm::vec3(0.0, 1.0, 0.0);
    }
    //BOTTOM
    else if (glm::abs(y - -0.5) < EPSILON) {
        return glm::vec3(0.0, -1.0, 0.0);
    }
    //BODY
    else {
        glm::vec3 n(x, 0, z);
        return glm::normalize(n);
    }
}
