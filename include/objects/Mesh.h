#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>
#include <iostream>

class Vertex;
class Edge;
class Face;

class Vertex {
private:
    glm::vec3 position;
    glm::vec3 normal;
    std::vector<Face*> faces;
public:
    Vertex(glm::vec3 pos) { position = pos; };
    ~Vertex() { };
    glm::vec3 getPos() { return position; };
    void setNormal(glm::vec3 norm) { normal = norm; };
    glm::vec3 getNormals() { return normal; };
    void addFace(Face* f) { faces.push_back(f); };
    std::vector<Face*> getFaces() { return faces; };
};

class Edge {
private:
    Vertex* src;
    Vertex* des;
    std::vector<Face*> faces;
public:
    Edge(Vertex* src, Vertex* des) { this->src = src; this->des = des; };
    ~Edge() { };
    Vertex* getSrc() { return src; };
    Vertex* getDes() { return des; };
    void addFace(Face* face) { faces.push_back(face); };
    std::vector<Face*>& getFaces() { return faces; };
};

class Face {
private:
    Vertex* vertices[3];;
    glm::vec3 faceNormal;
public:
    Face(Vertex* vertex1, Vertex* vertex2, Vertex* vertex3) {
        vertices[0] = vertex1;
        vertices[1] = vertex2;
        vertices[2] = vertex3;
        glm::vec3 v1 = vertex2->getPos() - vertex1->getPos();
        glm::vec3 v2 = vertex3->getPos() - vertex2->getPos();
        faceNormal = glm::normalize(glm::cross(v1, v2));
    };
    ~Face() { };
    Vertex* const* getVertices() { return vertices; };
    glm::vec3 getFaceNormal() { return faceNormal; };
    float intersect(glm::vec3 origin, glm::vec3 direction, glm::vec3& normal) {
        // Möller-Trumbore
        glm::vec3 e1 = vertices[1]->getPos() - vertices[0]->getPos();
        glm::vec3 e2 = vertices[2]->getPos() - vertices[1]->getPos();
        glm::vec3 pvec = glm::cross(direction, e2);
        float det = glm::dot(e1, pvec);
        if (glm::abs(det) < 1e-8f) {
            return -1.0f;
        }
        float invDet = 1.0f / det;
        glm::vec3 tvec = origin - vertices[0]->getPos();
        float u = glm::dot(tvec, pvec) * invDet;
        if (u < 0.0f || u > 1.0f) {
            return -1.0f;
        }
        glm::vec3 qvec = glm::cross(tvec, e1);
        float v = glm::dot(direction, qvec) * invDet;
        if (v < 0.0f || v > 1.0f) {
            return -1.0f;
        }
        float t = glm::dot(e2, qvec) * invDet;
        if (t > 0.0f) {
            t = t * glm::length(direction);
            normal = faceNormal;
            return t;
        }
        return -1.0f;
    };
};

class Mesh {
private:
    Vertex** vertices;
    Edge** edges;
    Face** faces;
    int lastV;
    int lastF;
public:
    Mesh() { };
    Mesh(int vcount, int fcount) { vertices = new Vertex*[vcount]; faces = new Face*[fcount]; lastV = 0; lastF = 0; };
    ~Mesh() { clear(); };
    void addVertex(Vertex* v);
    void addEdge(Edge* e);
    void addFace(Face* f);
    Vertex** getVertices();
    Vertex* getVertexAt(glm::vec3 pos);
    Edge** getEdges();
    Face** getFaces();
    void clear();
    class MeshVertexIterator {
    private:
        Mesh* m;
    public:
        MeshVertexIterator(Mesh* m) { this->m = m; };
        ~MeshVertexIterator() { };
        Vertex** begin() const { return this->m->beginV(); };
        Vertex** end() const { return this->m->endV(); };
    };
    MeshVertexIterator* getVertexIterator() { return new MeshVertexIterator(this); };
    class MeshFaceIterator {
    private:
        Mesh* m;
    public:
        MeshFaceIterator(Mesh* m) { this->m = m; };
        ~MeshFaceIterator() { };
        Face** begin() const { return this->m->beginF(); };
        Face** end() const { return this->m->endF(); };
    };
    MeshFaceIterator* getFaceIterator() { return new MeshFaceIterator(this); };
    Vertex** beginV() const { return vertices; };
    Vertex** endV() const { return vertices + lastV; };
    Face** beginF() const { return faces; };
    Face** endF() const { return faces + lastF; };
    void calculateVertexNormal();
    Mesh* rotate(float angle_x, float angle_y, float angle_z);
    Mesh* transform(glm::mat4 transformation);
    static Mesh* union_graph(std::vector<Mesh*>& graphs);
    static int convertVec3ToInt(glm::vec3 vec);
    void drawTriangleMeshFromFaces();
    void drawNormalsFromFaces();
};

#endif
