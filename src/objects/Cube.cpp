#include "objects/Cube.h"
#include "objects/Mesh.h"
#include <iostream>

// using namespace std;

static std::vector<Vertex> vertices;
static std::vector<Face>   Faces;


void Cube::drawTriangleMeshFromFaces() {
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

void Cube::draw() {
    drawTriangleMeshFromFaces();

}

void Cube::drawNormal() {
    glColor3f(1.0f, .0f, .0f);

    drawNormalForSingleFace();
}

void Cube::drawNormalForSingleFace() {
    glBegin(GL_LINES);
    for (Mesh* g : this->graphs) {
        for (Vertex* v : *g->getVertexIterator()) {
            const glm::vec3& normal = v->getNormals();
            const glm::vec3& pos = (v->getPos());

            // Draw normal vector from the vertex position
            glVertex3f(pos.x, pos.y, pos.z);
            glVertex3f(pos.x + normal.x * 0.1f, pos.y + normal.y * 0.1f, pos.z + normal.z * 0.1f);
        }
    }
    glEnd();
}

void Cube::calculate() {
    int vcount = (m_segmentsX + 1) * (m_segmentsY + 1);
    int fcount = m_segmentsX * m_segmentsY * 2;

    float stepX = 1.0f / m_segmentsX;
    float stepY = 1.0f / m_segmentsY;

    // Define transformation matrices for six faces
    glm::mat4 faceTransforms[6] = {
        // Front face (z = 0.5)
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.5f)),
        // Back face (z = -0.5)
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -0.5f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        // Right face (x = 0.5)
        glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, 0.0f, 0.0f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        // Left face (x = -0.5)
        glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, 0.0f, 0.0f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        // Top face (y = 0.5)
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.0f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        // Bottom face (y = -0.5)
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.5f, 0.0f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f))
    };

    // Define normal vectors for each face
    glm::vec3 faceNormals[6] = {
        glm::vec3(0.0f, 0.0f, 1.0f),   // Front normal
        glm::vec3(0.0f, 0.0f, -1.0f),  // Back normal
        glm::vec3(1.0f, 0.0f, 0.0f),   // Right normal
        glm::vec3(-1.0f, 0.0f, 0.0f),  // Left normal
        glm::vec3(0.0f, 1.0f, 0.0f),   // Top normal
        glm::vec3(0.0f, -1.0f, 0.0f)   // Bottom normal
    };

    this->clearGraphs();  // Clear existing meshes

    // Create mesh for each face
    for (int face = 0; face < 6; face++) {
        Mesh* g = new Mesh(vcount, fcount);
        
        // Calculate vertices
        for (int i = 0; i <= m_segmentsX; i++) {
            for (int j = 0; j <= m_segmentsY; j++) {
                float x = i * stepX - 0.5f;
                float y = j * stepY - 0.5f;
                float z = 0.0f;

                // Apply transformation matrix to vertex position
                glm::vec4 transformedPos = faceTransforms[face] * glm::vec4(x, y, z, 1.0f);
                glm::vec3 position(transformedPos.x, transformedPos.y, transformedPos.z);
                
                Vertex* v = new Vertex(position);
                v->setNormal(faceNormals[face]);  // Set corresponding face normal
                g->addVertex(v);
            }
        }

        // Calculate triangular faces
        for (int i = 0; i < m_segmentsX; i++) {
            for (int j = 0; j < m_segmentsY; j++) {
                // Calculate vertex indices for the current quad
                int index1 = i * (m_segmentsY + 1) + j;      // Bottom-left
                int index2 = index1 + 1;                      // Bottom-right
                int index3 = (i + 1) * (m_segmentsY + 1) + j; // Top-left
                int index4 = index3 + 1;                      // Top-right

                auto vertices = g->getVertices();
                // Create two triangles for each quad
                g->addFace(new Face(vertices[index1], vertices[index3], vertices[index4]));
                g->addFace(new Face(vertices[index1], vertices[index4], vertices[index2]));
            }
        }

        this->graphs.push_back(g);
    }
}

float Cube::intersect(glm::vec4 ray, glm::vec4 eye) {
    float t = -1.0f;
    float tmpts[6];
    tmpts[0] = (0.5f - eye.x) / ray.x;
    tmpts[1] = (-0.5f - eye.x) / ray.x;
    tmpts[2] = (0.5f - eye.y) / ray.y;
    tmpts[3] = (-0.5f - eye.y) / ray.y;
    tmpts[4] = (0.5f - eye.z) / ray.z;
    tmpts[5] = (-0.5f - eye.z) / ray.z;
    for (auto tmpt : tmpts) {
        float tmpx{ 0.0f }, tmpy{ 0.0f }, tmpz{ 0.0f };
        tmpx = eye.x + tmpt * ray.x;
        tmpy = eye.y + tmpt * ray.y;
        tmpz = eye.z + tmpt * ray.z;
        // std::cout << "x: " << tmpx << ", y: " << tmpy << ", z: " << tmpz << std::endl;
        if (abs(tmpx) < 0.5 + EPSILON && abs(tmpy) < 0.5 + EPSILON && abs(tmpz) < 0.5 + EPSILON) {
            // std::cout << "true!" << std::endl;
            if (tmpt > 0) {
                if (t < 0) t = tmpt;
                else t = MIN(tmpt, t);
            }
            // std::cout << t << std::endl;
        }
    }
    return t;
}

glm::vec2 Cube::getXYCoordinate(glm::vec4 ray, glm::vec4 eye,float t) {
    float x = eye.x + t * ray.x;
    float y = eye.y + t * ray.y;
    float z = eye.z + t * ray.z;

    bool pointOnRight = glm::abs(x - 0.5) < EPSILON;
    bool pointOnLeft = glm::abs(x + 0.5) < EPSILON;
    bool pointOnTop = glm::abs(y - 0.5) < EPSILON;
    bool pointOnBottom = glm::abs(y + 0.5) < EPSILON;
    bool pointOnFront = glm::abs(z - 0.5) < EPSILON;
    bool pointOnBack = glm::abs(z + 0.5) < EPSILON;

    // 6 faces
    if (pointOnRight) return glm::vec2(0.5-z, -0.5-y); //check
    if (pointOnLeft) return glm::vec2(0.5+z, 0.5+y);
    if (pointOnTop) return glm::vec2(0.5+x, 0.5-z); //check
    if (pointOnBottom) return glm::vec2(0.5+x, 0.5+z);
    if (pointOnFront) return glm::vec2(0.5+x, -0.5-y); // check
    if (pointOnBack) return glm::vec2(0.5-x, 0.5+y);

    return glm::vec2(0.0, 0.0);
}

glm::vec3 Cube::getNormal(glm::vec4 ray, glm::vec4 eye,float t) {
    float x = eye.x + t * ray.x;
    float y = eye.y + t * ray.y;
    float z = eye.z + t * ray.z;

    bool pointOnRight = glm::abs(x - 0.5) < EPSILON;
    bool pointOnLeft = glm::abs(x + 0.5) < EPSILON;
    bool pointOnTop = glm::abs(y - 0.5) < EPSILON;
    bool pointOnBottom = glm::abs(y + 0.5) < EPSILON;
    bool pointOnFront = glm::abs(z - 0.5) < EPSILON;
    bool pointOnBack = glm::abs(z + 0.5) < EPSILON;

    // 6 faces
    if (pointOnRight) return glm::vec3(1.0, 0.0, 0.0);
    if (pointOnLeft) return glm::vec3(-1.0, 0.0, 0.0);
    if (pointOnTop) return glm::vec3(0.0, 1.0, 0.0);
    if (pointOnBottom) return glm::vec3(0.0, -1.0, 0.0);
    if (pointOnFront) return glm::vec3(0.0, 0.0, 1.0);
    if (pointOnBack) return glm::vec3(0.0, 0.0, -1.0);

    return glm::vec3(0.0, 0.0, 0.0);
}
