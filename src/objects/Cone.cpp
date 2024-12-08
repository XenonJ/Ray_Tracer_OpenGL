#include "objects/Cone.h"
#include "objects/Mesh.h"
#include <cmath>
#include <iostream>


static std::vector<Vertex> vertices;
static std::vector<Face>   Faces;

static float radius = 0.5f;

void Cone::drawTriangleMeshFromFaces(){

    // Get current mode
    GLint shadeModel;
    glGetIntegerv(GL_SHADE_MODEL, &shadeModel);

    // Draw side face
    glBegin(GL_TRIANGLES);

    for (Mesh* g : this->graphs){
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
    

    

void Cone::drawNormalsFromFaces(){
    // glColor3f(1.0f, .0f, .0f);
    // glBegin(GL_LINES);
    // for (Face &face : Faces) {
    //     Vertex *const *verts      = face.getVertices();
    //     glm::vec3      v0Pos      = verts[0]->getPos();
    //     glm::vec3      v1Pos      = verts[1]->getPos();
    //     glm::vec3      v2Pos      = verts[2]->getPos();
    //     glm::vec3      v1v0       = glm::normalize(v1Pos - v0Pos);
    //     glm::vec3      v2v0       = glm::normalize(v2Pos - v0Pos);
    //     glm::vec3      faceNormal = glm::normalize(glm::cross(v1v0, v2v0));

    //     for (int i = 0; i < 3; ++i) {
    //         const glm::vec3 &pos = (verts[i]->getPos());
    //         glVertex3f(pos.x, pos.y, pos.z);
    //         glVertex3f(pos.x + faceNormal.x * .1f, pos.y + faceNormal.y * .1f,
    //                    pos.z + faceNormal.z * .1f);
    //     }
    // }
    // glEnd();
}


void Cone::draw(){
    drawTriangleMeshFromFaces();
}

void Cone::drawNormal() {
    // drawNormalsFromFaces();
    // std::cout << "The drawnormal function is called !" << std::endl;
    glColor3f(1.0f, .0f, .0f);

    glBegin(GL_LINES);
    for (Mesh* g : this->graphs){
        for (Vertex *v : *g->getVertexIterator()){
            const glm::vec3 &normal = v->getNormals();
            const glm::vec3 &pos = (v->getPos());
            
            glVertex3f(pos.x, pos.y, pos.z);
            glVertex3f(pos.x + normal.x * .1f, pos.y + normal.y * .1f,
                        pos.z + normal.z * .1f);
        }
    }
    glEnd();
}

void Cone::calculate() {
    int vcount = (m_segmentsX + 1) * (m_segmentsY + 1);
    int fcount = m_segmentsX  * (m_segmentsY + 1) * 2;
    // Create a new mesh to store the cone
    Mesh* side = new Mesh(vcount, fcount);
    Mesh* bottom = new Mesh(m_segmentsX + 1, m_segmentsX + 1);
    this->clearGraphs();

    float stepAngle = 2.0f * glm::pi<float>() / m_segmentsX;  // 360 degrees divided by segmentsX
    float stepY = 1.0f / m_segmentsY;  // Height divided by segmentsY

    float height = 1.0f;  // Total height of the cone
    float radius = 0.5f;  // Base radius of the cone

    // Generate vertices for the side mesh, based on segmentY and segmentX
    for (int i = 0; i <= m_segmentsY; i++) {
        float rate = i * stepY;
        float y = -height / 2 + i * stepY * height;  // y-coordinate for each level
        float r = radius * (height / 2 - y) / height;  // Calculate radius for current height level

        for (int j = 0; j < m_segmentsX; j++) {
            float angle = j * stepAngle;
            float x = r * glm::cos(angle);  // x-coordinate
            float z = r * glm::sin(angle);  // z-coordinate

            glm::vec3 position(x, y, z);
            Vertex* v = new Vertex(position);

            // Check if the vertex is on the bottom (y == -height / 2)
            if (y == -height / 2) {
                // This vertex is on the bottom, assign the downward normal
                glm::vec3 bottomNormal(0.0f, -1.0f, 0.0f);
                Vertex* tempV = new Vertex(position);
                tempV->setNormal(bottomNormal);  // Assign the normal to the vertex
                bottom->addVertex(tempV);
            }
            if (y != height / 2){
                float n_y = float(radius / sqrt(radius*radius + height*height));
                // Calculate normal for this vertex
                float Nx = x;
                float Nz = z;
                float length = sqrt(Nx * Nx + Nz * Nz);  // Only normalize x and z
                Nx /= length;
                Nz /= length;
                glm::vec3 normal(Nx, n_y , Nz);
                // normal = glm::normalize(normal);  // Normalize the normal vector
                v->setNormal(normal);  // Store the normal directly in the vertex
            }

            
            side->addVertex(v);
        }
    }

    // Add the top vertex (apex of the cone)
    glm::vec3 topVertexPos(0.0f, height / 2, 0.0f);
    Vertex* topVertex = new Vertex(topVertexPos);
    side->addVertex(topVertex);
    
    Vertex** verts = side->getVertices();
    // Generate side faces using index-based method
    for (int i = 0; i < m_segmentsY; ++i) {
        for (int j = 0; j < m_segmentsX; ++j) {
            // Calculate indices for the current and next vertices
            int index1 = i * m_segmentsX + j;
            int index2 = index1 + 1;
            if (j == m_segmentsX - 1) index2 = i * m_segmentsX;  // Wrap around to first vertex

            int index3 = (i + 1) * m_segmentsX + j;
            int index4 = index3 + 1;
            if (j == m_segmentsX - 1) index4 = (i + 1) * m_segmentsX;  // Wrap around

            // Create two faces for each segment
            Face* f1 = new Face(verts[index1], verts[index3], verts[index4]);
            Face* f2 = new Face(verts[index4], verts[index2], verts[index1]);

            side->addFace(f1);
            side->addFace(f2);
        }
    }

    // Create the top face that connects the last ring to the top vertex
    for (int j = 0; j < m_segmentsX; ++j) {
        int index1 = m_segmentsY * m_segmentsX + j;  // Last ring of vertices
        int index2 = m_segmentsY * m_segmentsX + (j + 1) % m_segmentsX;  // Wrap around

        // Create a face that connects the top vertex to the last ring
        Face* f = new Face(verts[index1], topVertex, verts[index2]);
        side->addFace(f);
    }

    // Add bottom mesh
    glm::vec3 bottomVertexPos(0.0f, -height / 2, 0.0f);
    Vertex* bottomVertex = new Vertex(bottomVertexPos);
    bottomVertex->setNormal(glm::vec3(0.0f, -1.0f, 0.0f));
    bottom->addVertex(bottomVertex);

    // Create the bottom face by connecting each segment to the center
    for (int j = 0; j < m_segmentsX; ++j) {
        int index1 = j;
        int index2 = (j + 1) % m_segmentsX;  // Wrap around

        Face* bottomFace = new Face(bottomVertex, verts[index2], verts[index1]);
        bottom->addFace(bottomFace);
    }

    // Add bottom and side meshes to the graphs
    this->graphs.push_back(side);
    this->graphs.push_back(bottom);

    // Calculate normals for shading
    // for (Mesh* g : this->graphs) {
    //     g->calculateVertexNormal();
    // }

    // Optional: Print total number of vertices and faces
    // int verticesSize = 0, facesSize = 0;
    // for (Mesh* g : this->graphs) {
    //     verticesSize += g->getVertices().size();
    //     facesSize += g->getFaces().size();
    // }
    // std::cout << "Vertices count: " << verticesSize << std::endl;
    // std::cout << "Faces count: " << facesSize << std::endl;
}

float Cone::intersect(glm::vec4 ray, glm::vec4 eye) {
    // std::cout << "cone" << std::endl;
    float t = -1.0f;
    // BODY
    float A = 16 * ray.x * ray.x - 4 * ray.y * ray.y + 16 * ray.z * ray.z;
    float B = 32 * eye.x * ray.x - 8 * eye.y * ray.y + 32 * eye.z * ray.z + 4 * ray.y;
    float C = 16 * eye.x * eye.x - 4 * eye.y * eye.y + 16 * eye.z * eye.z + 4 * eye.y - 1;
    float delta = B * B - 4 * A * C;
    if (delta > 0.0f) {
        float t1 = (-B + glm::sqrt(delta)) / (2 * A);
        float t2 = (-B - glm::sqrt(delta)) / (2 * A);
        // choose a feasible t
        if (t1 >= 0 && t2 >= 0) {
            // check if feasible t is valid
            glm::vec3 intersectPoint1 = eye + t1 * ray;
            glm::vec3 intersectPoint2 = eye + t2 * ray;
            bool point1_is_valid = intersectPoint1.y >= -0.5 && intersectPoint1.y <= 0.5;
            bool point2_is_valid = intersectPoint2.y >= -0.5 && intersectPoint2.y <= 0.5;
            if (point1_is_valid && point2_is_valid) { // ray come from outside the bicone, we'll keep the CLOSER one
                t = MIN(t1, t2);
            } else if (point1_is_valid && !point2_is_valid) { // ray come from inside the bicone, we'll keep the VALID one
                t = t1;
            } else if (!point1_is_valid && point2_is_valid) { // ray come from inside the bicone, we'll keep the VALID one
                t = t2;
            } else { // simply no valid intersection point
                t = -1.0f;
            }
        }
        else if (t1 >= 0 && t2 < 0) {
            t = t1;
            // check if feasible t is valid
            glm::vec3 intersectPoint = eye + t * ray;
            if (intersectPoint.y < -0.5 || intersectPoint.y > 0.5) {
                t = -1.0f;
            }
        }
        else if (t1 < 0 && t2 >= 0) {
            t = t2;
            // check if feasible t is valid
            glm::vec3 intersectPoint = eye + t * ray;
            if (intersectPoint.y < -0.5 || intersectPoint.y > 0.5) {
                t = -1.0f;
            }
        }
    }
    // CAP
    float tmpt = (-0.5f - eye.y) / ray.y;
    float tmpx{ 0.0f }, tmpy{ 0.0f }, tmpz{ 0.0f };
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

glm::vec2 Cone::getXYCoordinate(glm::vec4 ray, glm::vec4 eye,float t) {
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
    
    //BOTTOM
    if (glm::abs(y - -0.5) < EPSILON) {
        return glm::vec2(x+0.5, -0.5+z);
    }
    //BODY
    else {
        return glm::vec2(u, v);
    }

}

glm::vec3 Cone::getNormal(glm::vec4 ray, glm::vec4 eye, float t) {
    //if intersect point is on the cap
    if (glm::abs((eye.y + t * ray.y) - -0.5) < EPSILON) {
        return glm::vec3(0.0, -1.0, 0.0);
    }
    //if intersect point is on the body
    else {
        float x = eye.x + t * ray.x;
        float z = eye.z + t * ray.z;
        glm::vec3 n(x, glm::sqrt(x * x + z * z) / 2, z);
        return glm::normalize(n);
    }
}