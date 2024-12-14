#include "objects/SceneGraph.h"

KDTreeNode* KDTree::build(std::vector<SceneGraphNode*>& objects, int depth) {
    if (objects.empty()) return nullptr;

    // loop through axis x y z
    int axis = depth % 3;

    AABB nodeBox = computeBoundingBox(objects);

    // leaf node
    if (objects.size() <= 2) {
        return new KDTreeNode(nodeBox, objects);
    }

    // sort with axis
    std::sort(objects.begin(), objects.end(), [axis](SceneGraphNode* a, SceneGraphNode* b) {
        return a->getTransformationMat()[3][axis] < b->getTransformationMat()[3][axis]; // translation in transformationMAt
    });

    // split
    size_t mid = objects.size() / 2;
    std::vector<SceneGraphNode*> leftObjects(objects.begin(), objects.begin() + mid);
    std::vector<SceneGraphNode*> rightObjects(objects.begin() + mid, objects.end());

    // generate children
    KDTreeNode* node = new KDTreeNode(nodeBox);
    node->left = build(leftObjects, depth + 1);
    node->right = build(rightObjects, depth + 1);

    return node;
}

AABB KDTree::computeBoundingBox(const std::vector<SceneGraphNode*>& objects) {
    glm::vec3 min(FLT_MAX), max(-FLT_MAX);

    for (auto obj : objects) {
        AABB box = AABB::transformCubeAABB(obj->getTransformationMat());
        min = glm::min(min, box.min);
        max = glm::max(max, box.max);
    }

    return AABB(min, max);
}

AABB AABB::transformCubeAABB(const glm::mat4& transformationMat) {
    // translation
    glm::vec3 translation = glm::vec3(transformationMat[3]);

    // rotation and scaling
    glm::vec3 axisX = glm::vec3(transformationMat[0]);
    glm::vec3 axisY = glm::vec3(transformationMat[1]);
    glm::vec3 axisZ = glm::vec3(transformationMat[2]);

    // half extent for unit cube
    glm::vec3 halfExtents = {0.5f, 0.5f, 0.5f};

    // half extent in world space
    glm::vec3 worldHalfExtents = glm::abs(axisX) * halfExtents.x +
                                 glm::abs(axisY) * halfExtents.y +
                                 glm::abs(axisZ) * halfExtents.z;

    glm::vec3 worldMin = translation - worldHalfExtents;
    glm::vec3 worldMax = translation + worldHalfExtents;

    return AABB(worldMin, worldMax);
}

glm::mat4 AABB::getTransformationMat() {
    glm::mat4 ret(1.0f);
    glm::vec3 scale = this->max - this->min;
    glm::vec3 translation = (this->min + this->max) / 2.0f;
    ret[0][0] = scale[0];
    ret[1][1] = scale[1];
    ret[2][2] = scale[2];
    ret[3][0] = translation[0];
    ret[3][1] = translation[1];
    ret[3][2] = translation[2];
    return ret;
}

float SceneGraphNode::intersect(glm::vec3 origin, glm::vec3 direction, glm::vec3& normal) {
    float t = -1.0f;
    for (auto mesh : this->shape->graphs) {
        for (Face* f : *mesh->getFaceIterator()) {
            float tmpt = f->intersect(origin, direction, normal);
            if (tmpt > 0) {
                if (t < 0) t = tmpt;
                else t = MIN(tmpt, t);
            }
        }
    }
    return t;
}

/*
    array structure
     1 -  9: vertices
    10 - 12: face normal
    13 - 15: rgb
    16 - 18: mesh type
*/
bool SceneGraphNode::buildArray(std::vector<float>& array) {
    // printf("transformation mat: \n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n",
    //     this->getTransformationMat()[0][0], this->getTransformationMat()[0][1], this->getTransformationMat()[0][2], this->getTransformationMat()[0][3], 
    //     this->getTransformationMat()[1][0], this->getTransformationMat()[1][1], this->getTransformationMat()[1][2], this->getTransformationMat()[1][3], 
    //     this->getTransformationMat()[2][0], this->getTransformationMat()[2][1], this->getTransformationMat()[2][2], this->getTransformationMat()[2][3], 
    //     this->getTransformationMat()[3][0], this->getTransformationMat()[3][1], this->getTransformationMat()[3][2], this->getTransformationMat()[3][3]
    // );
    for (auto mesh : this->shape->graphs) {
        int i = 0;
        for (Face* f : *mesh->getFaceIterator()) {
            Vertex* const* v = f->getVertices();
            // 1 - 9
            for (int i = 0; i < 3; i++) {
                glm::vec3 worldPosition = this->getTransformationMat() * glm::vec4(v[i]->getPos(), 1.0f);
                // printf("x, y, z: %f, %f, %f\n", worldPosition.x, worldPosition.y, worldPosition.z);
                array.push_back(worldPosition.x);
                array.push_back(worldPosition.y);
                array.push_back(worldPosition.z);
            }
            // 10 - 12
            array.push_back(f->getFaceNormal().x);
            array.push_back(f->getFaceNormal().y);
            array.push_back(f->getFaceNormal().z);
            // 13 - 15
            array.push_back(this->material.cDiffuse.r);
            array.push_back(this->material.cDiffuse.g);
            array.push_back(this->material.cDiffuse.b);
            // 16 - 18
            array.push_back(1.0f);
            array.push_back(0.0f);
            array.push_back(0.0f);
        }
    }
    return true;
}

bool SceneGraph::buildArray(std::vector<float>& array) {
    for (auto node : this->list) {
        if (!node->buildArray(array)) {
            return false;
        }
    }
    return true;
}

void meshKDTree::build(const std::vector<float>& array, int max_triangles_per_leaf) {
    vector<int> indexes(array.size() / 18);
    for (int i = 0; i < indexes.size(); i++) { indexes[i] = i; }
    rootIndex = buildRec(array, indexes, 0, max_triangles_per_leaf);
}

int meshKDTree::buildRec(const std::vector<float>& array, std::vector<int>& faceIndexes, int depth, int max_triangles_per_leaf) {
    meshKDTreeNode node;

    // compute AABB
    for (int index : faceIndexes) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                float coord = array[index * 18 + i * 3 + j];
                node.min_xyz[j] = std::min(coord, node.min_xyz[j]);
                node.max_xyz[j] = std::max(coord, node.max_xyz[j]);
            }
        }
    }

    // leaf node
    if (faceIndexes.size() <= max_triangles_per_leaf) {
        for (int i : faceIndexes) { node.faceIndexes.push_back(i); }
        // node.faceIndexes = faceIndexes;
        nodes.push_back(node);
        return nodes.size() - 1;
    }

    // split meshes
    int axis = depth % 3;
    vector<float> centers;
    for (int i : faceIndexes) { centers.push_back((array[i * 18 + 0 + axis] + array[i * 18 + 3 + axis] + array[i * 18 + 6 + axis]) / 3); }
    std::sort(centers.begin(), centers.end());
    float mid = centers[centers.size() / 2];
    // float mid = (node.min_xyz[axis] + node.max_xyz[axis]) / 2;
    // printf("mid: %f\n", mid);
    std::vector<int> left_indexes, right_indexes;
    for (int index : faceIndexes) {
        float center = (array[index * 18 + 0 + axis] + array[index * 18 + 3 + axis] + array[index * 18 + 6 + axis]) / 3;
        if (center < mid) {
            left_indexes.push_back(index);
        }
        else {
            right_indexes.push_back(index);
        }
    }
    // create left and right node
    node.left = buildRec(array, left_indexes, depth + 1, max_triangles_per_leaf);
    node.right = buildRec(array, right_indexes, depth + 1, max_triangles_per_leaf);

    nodes.push_back(node);
    return nodes.size() - 1;
}

/*
     1 -  3: left, right, 0.0f
     4 -  6: min_xyz
     7 -  9: max_xyz
    10 - 15: nodes(only in leaf nodes)
*/
void meshKDTree::buildArray(std::vector<float>& array) {
    for (auto node : nodes) {
        // 1 - 3
        array.push_back(node.left);
        array.push_back(node.right);
        array.push_back(0.0f);
        // 4 - 6
        array.push_back(node.min_xyz[0]);
        array.push_back(node.min_xyz[1]);
        array.push_back(node.min_xyz[2]);
        // 7 - 9
        array.push_back(node.max_xyz[0]);
        array.push_back(node.max_xyz[1]);
        array.push_back(node.max_xyz[2]);
        // 10 - 15
        if (node.left == -1) {
            int size = std::min(int(node.faceIndexes.size()), 6);
            for (int i = 0; i < size; i++) {
                array.push_back(node.faceIndexes[i]);
            }
            // place holder
            for (int j = size; j < 6; j++) {
                array.push_back(-1.0f);
            }
        }
        else {  // -1.0f if not leaf node
            array.push_back(-1.0f);
            array.push_back(-1.0f);
            array.push_back(-1.0f);
            array.push_back(-1.0f);
            array.push_back(-1.0f);
            array.push_back(-1.0f);
        }
    }
}
