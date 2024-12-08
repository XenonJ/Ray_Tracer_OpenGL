#include "objects/Sphere.h"
#include "objects/Mesh.h"
#include <iostream>


static std::vector<Vertex> vertices;
static std::vector<Face>   Faces;

static float radius = 0.5f;

void Sphere::drawTriangleMeshFromFaces() {

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
            // glNormal3f(normal.x, normal.y, normal.z);
        }
    }
    glEnd();

}




void Sphere::drawNormalsFromFaces() {
    glColor3f(1.0f, .0f, .0f);
    glBegin(GL_LINES);
    for (Face& face : Faces) {
        Vertex* const* verts = face.getVertices();
        glm::vec3      v0Pos = verts[0]->getPos();
        glm::vec3      v1Pos = verts[1]->getPos();
        glm::vec3      v2Pos = verts[2]->getPos();
        glm::vec3      v1v0 = glm::normalize(v1Pos - v0Pos);
        glm::vec3      v2v0 = glm::normalize(v2Pos - v0Pos);
        glm::vec3      faceNormal = glm::normalize(glm::cross(v1v0, v2v0));

        for (int i = 0; i < 3; ++i) {
            const glm::vec3& pos = (verts[i]->getPos());
            glVertex3f(pos.x, pos.y, pos.z);
            glVertex3f(pos.x + faceNormal.x * .1f, pos.y + faceNormal.y * .1f,
                pos.z + faceNormal.z * .1f);
        }
    }
    glEnd();
}


void Sphere::draw() {
    drawTriangleMeshFromFaces();
}

void Sphere::drawNormal() {
    // drawNormalsFromFaces();
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

void Sphere::calculate() {
    int vcount = m_segmentsX * m_segmentsY + 2;
    int fcount = m_segmentsX * (m_segmentsY - 1) * 2;
    // Create a new graph to store the sphere surface
    Mesh* sphere = new Mesh(vcount, fcount);

    this->clearGraphs();

    float stepLongitude = 2.0f * glm::pi<float>() / m_segmentsX;  // Longitude angle step (radians)
    float stepLatitude = glm::pi<float>() / m_segmentsY;  // Latitude angle step (radians)

    // Store all vertices in the sphere
    std::vector<Vertex*> tempVerts;

    // Generate vertices from southern point to northern point
    for (int i = 1; i < m_segmentsY; i++) {  // Skipping poles (i = 0 for south, i = m_segmentsY for north)
        float latAngle = -glm::half_pi<float>() + i * stepLatitude;  // Latitude angle from -π/2 to π/2
        float sinLat = glm::sin(latAngle);
        float cosLat = glm::cos(latAngle);

        for (int j = 0; j < m_segmentsX; j++) {  // Full rotation in longitude
            float lonAngle = j * stepLongitude;  // Longitude angle
            float sinLon = glm::sin(lonAngle);
            float cosLon = glm::cos(lonAngle);

            // Transform Sphere coordinate to Cartesian coordinate
            float x = radius * cosLat * cosLon;
            float y = radius * sinLat;
            float z = radius * cosLat * sinLon;

            glm::vec3 position(x, y, z);

            // Create vertex
            Vertex* v = new Vertex(position);

            // Set the normal to be the normalized position vector (direction from the origin)
            glm::vec3 normal = glm::normalize(position);
            v->setNormal(normal);  // Set normal for this vertex

            // Add vertex to the mesh and store in temporary list
            sphere->addVertex(v);
            tempVerts.push_back(v);  // Store vertex in tempVerts
        }
    }

    // Add poles (north and south vertices)
    glm::vec3 topPosition(0.0f, radius, 0.0f);      // North pole
    glm::vec3 bottomPosition(0.0f, -radius, 0.0f);  // South pole
    Vertex* topv = new Vertex(topPosition);
    Vertex* bottomv = new Vertex(bottomPosition);

    // Set normals for poles
    topv->setNormal(glm::vec3(0.0f, 1.0f, 0.0f));     // North pole normal points upwards
    bottomv->setNormal(glm::vec3(0.0f, -1.0f, 0.0f)); // South pole normal points downwards

    // Add poles to the mesh
    sphere->addVertex(bottomv);  // South pole
    sphere->addVertex(topv);     // North pole

    int bottomIndex = tempVerts.size();  // Index for south pole
    int topIndex = bottomIndex + 1;      // Index for north pole

    // Build faces for the sphere
    for (int i = 0; i < m_segmentsY - 1; i++) {
        for (int j = 0; j < m_segmentsX; j++) {
            int nextJ = (j + 1) % m_segmentsX;  // Wrap around in longitude

            // First row: connect bottom (south pole)
            if (i == 0) {
                Face* bottomFace = new Face(sphere->getVertices()[bottomIndex], tempVerts[j], tempVerts[nextJ]);
                sphere->addFace(bottomFace);
            }
            // Last row: connect top (north pole)
            else if (i == m_segmentsY - 2) {
                int baseIndex = (i)*m_segmentsX;
                int baseIndex1 = (i - 1) * m_segmentsX;
                int baseIndex2 = i * m_segmentsX;

                Face* f1 = new Face(tempVerts[baseIndex1 + j], tempVerts[baseIndex2 + j], tempVerts[baseIndex2 + nextJ]);
                Face* f2 = new Face(tempVerts[baseIndex2 + nextJ], tempVerts[baseIndex1 + nextJ], tempVerts[baseIndex1 + j]);

                sphere->addFace(f1);
                sphere->addFace(f2);
                Face* topFace = new Face(sphere->getVertices()[topIndex], tempVerts[baseIndex + nextJ], tempVerts[baseIndex + j]);
                sphere->addFace(topFace);
            }
            // Middle rows: connect latitude strips
            else {
                int baseIndex1 = (i - 1) * m_segmentsX;
                int baseIndex2 = i * m_segmentsX;

                Face* f1 = new Face(tempVerts[baseIndex1 + j], tempVerts[baseIndex2 + j], tempVerts[baseIndex2 + nextJ]);
                Face* f2 = new Face(tempVerts[baseIndex2 + nextJ], tempVerts[baseIndex1 + nextJ], tempVerts[baseIndex1 + j]);

                sphere->addFace(f1);
                sphere->addFace(f2);
            }
        }
    }

    // Add mesh to graphs
    this->graphs.push_back(sphere);

    // Optional: Output vertex and face counts
    // std::cout << "Vertices count: " << sphere->getVertices().size() << std::endl;
    // std::cout << "Faces count: " << sphere->getFaces().size() << std::endl;
}

float Sphere::intersect(glm::vec4 rayV, glm::vec4 eyePointP) {
    float t = -1.0f;
    float A = glm::dot(glm::vec3(rayV), glm::vec3(rayV));
    float B = 2 * glm::dot(glm::vec3(eyePointP), glm::vec3(rayV));
    float C = glm::dot(glm::vec3(eyePointP), glm::vec3(eyePointP)) - 0.25f;
    float delta = B * B - 4 * A * C;
    if (delta > 0) {
        float t1 = (-B - glm::sqrt(delta)) / (2 * A);
        float t2 = (-B + glm::sqrt(delta)) / (2 * A);
        if (t1 >= 0 && t2 >= 0) {
            t = MIN(t1, t2);
        }
        else if (t1 < 0 && t2 >= 0) {
            t = t2;
        }
        else if (t1 >= 0 && t2 < 0) {
            t = t1;
        }
    }
    return t;
}

glm::vec2 Sphere::getXYCoordinate(glm::vec4 ray, glm::vec4 eye,float t) {
    float x = eye.x + t * ray.x;
    float y = eye.y + t * ray.y;
    float z = eye.z + t * ray.z;

    float theta = PI + atan2(z, x);  // Dont know why but it works
    float phi = std::asin(y / 0.5);
    float u = 0.0;
    float v = - (phi/PI + 0.5);  // Dont know why but it works
    
    if (theta < 0) {
        u = - (theta / (2*PI));
    } else {
        u = 1 - (theta / (2*PI));
    }
    return glm::vec2(u, v);
}

glm::vec3 Sphere::getNormal(glm::vec4 rayV, glm::vec4 eyePointP, float t) {
    return glm::normalize(eyePointP + t * rayV);
}
