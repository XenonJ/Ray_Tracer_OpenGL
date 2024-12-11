#ifndef SCENEGRAPH_H
#define SCENEGRAPH_H

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Shape.h"
#include <FL/gl.h>
#include <FL/glu.h>
#include <vector>
#include "scene/SceneParser.h"

class SceneGraphNode;
class SceneGraph;
class KDTreeNode;
class KDTree;
class AABB; // axis-aligned bounding box

class AABB {
public:
    glm::vec3 min;
    glm::vec3 max;

    AABB(const glm::vec3& min, const glm::vec3& max) { this->min = min; this->max = max; };
    static AABB transformCubeAABB(const glm::mat4& transformationMat);
    glm::mat4 getTransformationMat();
private:

};

class KDTreeNode {
public:
    AABB boundingBox;
    KDTreeNode* left;
    KDTreeNode* right;
    std::vector<SceneGraphNode*> objects;

    KDTreeNode(const AABB& box) : boundingBox(box), left(nullptr), right(nullptr) { };
    KDTreeNode(const AABB& box, std::vector<SceneGraphNode*>& objects) : boundingBox(box), left(nullptr), right(nullptr) { for (auto node : objects) { this->objects.push_back(node); } };
    ~KDTreeNode() { delete left; delete right; };
private:

};

class KDTree {
public:
    KDTree(std::vector<SceneGraphNode*>& objects) { this->root = build(objects, 0); };
    ~KDTree() { delete root; };
    KDTreeNode* getRoot() { return root; };
private:
    KDTreeNode* root;

    KDTreeNode* build(std::vector<SceneGraphNode*>& nodes, int depth);
    AABB computeBoundingBox(const std::vector<SceneGraphNode*>& objects);
};

class SceneGraphNode {
private:
    glm::mat4 transformationMat;
    Shape* shape;
    SceneMaterial material;
public:
    SceneGraphNode(glm::mat4 mat, Shape* sp, SceneMaterial material) { this->transformationMat = mat; this->shape = sp; this->material = material; };
    ~SceneGraphNode() { delete this->shape; };
    void setTransformation(glm::mat4 mat) { this->transformationMat = mat; };
    void draw() { glMultMatrixf(glm::value_ptr(this->transformationMat)); this->shape->draw(); };
    SceneMaterial getMaterial() { return this->material; };
    void setSegments(int segX, int segY) { this->shape->setSegments(segX, segY); };
    void calculate() { this->shape->calculate(); printf("shape segments: %d, %d\n", Shape::m_segmentsX, Shape::m_segmentsY); };
    glm::mat4 getTransformationMat() { return this->transformationMat; };
    OBJ_TYPE getShape() { return this->shape->getType(); };
    float intersect(glm::vec3 origin, glm::vec3 direction, glm::vec3& normal);
    bool buildArray(std::vector<float>& array);
};

class SceneGraph {
private:
    std::vector<SceneGraphNode*> list;
    KDTree* tree;
public:
    SceneGraph() { };
    ~SceneGraph() { clear(); };
    void addNode(SceneGraphNode* node) { this->list.push_back(node); };
    void clear() { for (auto node : this->list) { delete node; }; this->list.clear(); delete this->tree; this->tree = nullptr; };
    std::vector<SceneGraphNode*>::iterator getIterator() { return this->list.begin(); };
    std::vector<SceneGraphNode*>::iterator getEnd() { return this->list.end(); };
    void calculate() { for (auto node : this->list) { node->calculate(); } };
    void buildKDTree() { delete this->tree; this->tree = new KDTree(this->list); };
    KDTree* getKDTree() { return this->tree; };
    bool buildArray(std::vector<float>& array);
};

#endif