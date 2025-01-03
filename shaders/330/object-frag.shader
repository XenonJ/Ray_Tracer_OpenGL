#version 330

/* mesh buffer
    array structure
     1 -  9: vertices
    10 - 12: face normal
    13 - 15: rgb
    16 - 18: mesh type
*/
uniform int numMeshBuffers;
uniform int maxTrianglesPerBuffer;
uniform samplerBuffer meshBuffer[6];   // assume max 6 buffers
uniform int meshSize; // mesh count

/* tree buffer
     1 -  3: left, right, 0.0f
     4 -  6: min_xyz
     7 -  9: max_xyz
    10 - 15: nodes(only in leaf nodes)
*/
uniform int numTreeBuffers;
uniform int maxNodesPerBuffer;
uniform samplerBuffer treeBuffer[6];   // assume max 6 buffers
uniform int treeSize; // mesh count
uniform int rootIndex;  // index for kdtree root node

uniform int frameCounter;   // incr per frame

uniform vec3 lightPos;  // light position in world space

uniform vec3 meshTrans; // mesh translation

in vec3 pixelColor; // some background calculated by pixel(i, j)
in vec3 rayOrigin;  // camera position
in vec3 rayDirection;   // normalized direction for current ray

const float PI = 3.14159265359;
const int MAX_STACK_SIZE = 1000;

#define MAX_WAVES 219

uniform int waveCount;
uniform vec2 waveDir[MAX_WAVES];
uniform float waveOmega[MAX_WAVES];
uniform float waveAmplitude[MAX_WAVES];
uniform float wavePhaseOffset[MAX_WAVES];

layout(location = 0) out vec4 outColor;
layout(location = 1) out float outDistance;
// out vec4 outputColor;

struct mesh {
    vec3 v1, v2, v3;
    vec3 faceNormal;
    vec3 diffuseColor;
    vec3 type;
};

struct node {
    int left, right;
    vec3 min_xyz;
    vec3 max_xyz;
    int meshes[6];
};

vec2 intersectionAABB(vec3 boxMin, vec3 boxMax, vec3 origin, vec3 direction) {
    float tnear = -1e10;
    float tfar = 1e10;

    // Check each axis
    for (int i = 0; i < 3; i++) {
        if (direction[i] != 0.0) {
            float t1 = (boxMin[i] - origin[i]) / direction[i];
            float t2 = (boxMax[i] - origin[i]) / direction[i];

            if (t1 > t2) {
                float temp = t1;
                t1 = t2;
                t2 = temp;
            }

            tnear = max(tnear, t1);
            tfar = min(tfar, t2);

            if (tnear > tfar || tfar < 0.0) {
                return vec2(-1.0f); // No intersection
            }
        } else {
            // Ray is parallel to the slabs
            if (origin[i] < boxMin[i] || origin[i] > boxMax[i]) {
                return vec2(-1.0f); // Ray misses the box
            }
        }
    }

    return vec2(tnear, tfar);
}

float intersectionTriangle(mesh m, vec3 origin, vec3 direction) {
    vec3 e1 = m.v2 - m.v1;
    vec3 e2 = m.v3 - m.v1;
    vec3 pvec = cross(direction, e2);
    float det = dot(e1, pvec);
    if (abs(det) < 1e-6f) {
        return -1.0f;
    }
    float invDet = 1.0f / det;
    vec3 tvec = origin - m.v1;
    float u = dot(tvec, pvec) * invDet;
    if (u < 0.0f || u > 1.0f) {
        return -1.0f;
    }
    vec3 qvec = cross(tvec, e1);
    float v = dot(direction, qvec) * invDet;
    if (v < 0.0f || u + v > 1.0f) {
        return -1.0f;
    }
    float t = dot(e2, qvec) * invDet;
    if (t > 0.0f) {
        return t;
    }
    return -1.0f;
}

mesh getMesh(int index) {
    int bufferIdx = index / maxTrianglesPerBuffer;
    int localIdx = index % maxTrianglesPerBuffer;
    mesh ret;
    ret.v1 = texelFetch(meshBuffer[bufferIdx], 6 * localIdx).rgb;
    ret.v2 = texelFetch(meshBuffer[bufferIdx], 6 * localIdx + 1).rgb;
    ret.v3 = texelFetch(meshBuffer[bufferIdx], 6 * localIdx + 2).rgb;
    ret.faceNormal = texelFetch(meshBuffer[bufferIdx], 6 * localIdx + 3).rgb;
    ret.diffuseColor = texelFetch(meshBuffer[bufferIdx], 6 * localIdx + 4).rgb;
    ret.type = texelFetch(meshBuffer[bufferIdx], 6 * localIdx + 5).rgb;
    return ret;
}

node getNode(int index) {
    int bufferIdx = index / maxNodesPerBuffer;
    int localIdx = index % maxNodesPerBuffer;
    node ret;
    vec3 v1 = texelFetch(treeBuffer[bufferIdx], 5 * localIdx).rgb;
    ret.left = int(round(v1.x));
    ret.right = int(round(v1.y));
    ret.min_xyz = texelFetch(treeBuffer[bufferIdx], 5 * localIdx + 1).rgb;
    ret.max_xyz = texelFetch(treeBuffer[bufferIdx], 5 * localIdx + 2).rgb;
    vec3 v4 = texelFetch(treeBuffer[bufferIdx], 5 * localIdx + 3).rgb;
    vec3 v5 = texelFetch(treeBuffer[bufferIdx], 5 * localIdx + 4).rgb;
    ret.meshes[0] = int(round(v4.x));
    ret.meshes[1] = int(round(v4.y));
    ret.meshes[2] = int(round(v4.z));
    ret.meshes[3] = int(round(v5.x));
    ret.meshes[4] = int(round(v5.y));
    ret.meshes[5] = int(round(v5.z));
    return ret;
}

vec2 intersection(vec3 origin, vec3 direction) {
    int idx = -1;
    float t = -1.0f;
    for (int i = 0; i < meshSize; i++) {
        mesh m = getMesh(i);
        float tmpt = intersectionTriangle(m, origin, direction);
        if (t < 0.0f) {
            if (tmpt > 0.0f) {
                t = tmpt;
                idx = i;
            }
        }
        else {
            if (tmpt > 0 && tmpt < t) {
                t = tmpt;
                idx = i;
            }
        }
    }
    return vec2(idx, t);
}

vec2 intersectionKDTree(vec3 origin, vec3 direction) {
    // initialize stack with array
    int stack[MAX_STACK_SIZE];
    int stackSize = 0;

    // push root into stack
    stack[stackSize++] = rootIndex;

    int idx = -1;
    float t = -1.0f;

    while (stackSize > 0) {
        // pop top
        int currentIndex = stack[--stackSize];
        node n = getNode(currentIndex);

        if (n.left == -1) { // leaf node
            for (int j = 0; j < 6; j++) {
                if (n.meshes[j] == -1) break; // no more mesh
                mesh m = getMesh(n.meshes[j]);
                float tmpt = intersectionTriangle(m, origin, direction);
                if (t < 0.0f) {
                    if (tmpt > 0.0f) {
                        t = tmpt;
                        idx = n.meshes[j];
                    }
                } else {
                    if (tmpt > 0 && tmpt < t) {
                        t = tmpt;
                        idx = n.meshes[j];
                    }
                }
            }
        } else { // node
            node left = getNode(n.left);
            node right = getNode(n.right);

            if (intersectionAABB(left.min_xyz, left.max_xyz, origin, direction).x >= 0.0f) {
                stack[stackSize++] = n.left; // push left into stack
            }

            if (intersectionAABB(right.min_xyz, right.max_xyz, origin, direction).x >= 0.0f) {
                stack[stackSize++] = n.right; // push right into stack
            }
        }
    }

    return vec2(idx, t);
}

vec4 calculateRGB(vec3 origin, vec3 direction) {
    vec4 color = vec4(0.0f);
    // vec2 ret = intersection(rayOrigin, rayDirection);
    vec2 ret = intersectionKDTree(origin, direction);
    int idx = int(round(ret.x));
    float t = ret.y;
    if (idx < 0) {
        return vec4(0.0f);
    }
    // else {   // only intersection
    //     return vec4(1.0f);
    // }
    mesh m = getMesh(idx);
    vec3 worldPosition = origin + t * direction;
    color = vec4(m.diffuseColor * max(dot(normalize(lightPos - worldPosition), m.faceNormal), 0.0f), 1.0f);
    return color;
}

// sea box
#define sea_bottom -5
#define sea_top -4
#define sea_width 200



// The wave height is calculated using the equation:
//     H = A * sin(k · x - ωt + φ)
// where A is the amplitude, k is the wave direction, ω is the frequency, and φ is the phase offset.
float calculateWaveHeight(vec2 pos, float t) {
    float height = 0.0;
    for (int i = 0; i < waveCount; i++) {
        float phase = dot(waveDir[i], pos) * waveOmega[i] - waveOmega[i]*t + wavePhaseOffset[i];
        height += waveAmplitude[i] * sin(phase);
    }
    return height;
}

// This gradient(slope of the surface) defines the normal vector.
void computeNormalFromHeight(vec2 pos, float t, out float H, out vec3 N) {
    float epsilon = 0.001;
    float H0 = calculateWaveHeight(pos, t);
    float Hx = (calculateWaveHeight(pos + vec2(epsilon,0), t) - calculateWaveHeight(pos - vec2(epsilon,0), t)) / (2.0*epsilon);
    float Hz = (calculateWaveHeight(pos + vec2(0,epsilon), t) - calculateWaveHeight(pos - vec2(0,epsilon), t)) / (2.0*epsilon);
    H = H0;
    N = normalize(vec3(-Hx, abs(1.0), -Hz));
}

vec4 renderDynamicSea(vec3 cameraPosition, vec3 worldPosition, float depth)
{
    vec3 viewDirection = normalize(worldPosition - cameraPosition);
    vec3 boxMin = vec3(-sea_width, sea_bottom, -sea_width);
    vec3 boxMax = vec3(sea_width, sea_top, sea_width);
    float tnear = intersectionAABB(boxMin, boxMax, cameraPosition, viewDirection).x;
    if (abs(tnear - -1.0f) < 1e-8f || (depth > 0.0f && tnear > depth)) {
        return vec4(0.0f);
    }
    vec3 point = cameraPosition + viewDirection * max(tnear, 0.0);

    vec2 pos = vec2(point.x, point.z);
    float time = float(frameCounter)*0.05; // dynamic time

    float H;
    vec3 normal;
    computeNormalFromHeight(pos, time, H, normal);

    float diffuse;

    // compute shadow from mesh
    vec3 rayToLight = normalize(lightPos - point);
    mat4 mat = mat4(1.0f);
    mat[3] = vec4(meshTrans, 1.0f);
    vec3 origin = (inverse(mat) * vec4(point, 1.0f)).xyz;
    vec3 direction = (inverse(mat) * vec4(rayToLight, 0.0f)).xyz;
    vec2 intersectionToLight = intersectionKDTree(origin, direction);
    if (intersectionToLight.x > 0.0f) {
        diffuse = 0.0f;
    }
    else {
        diffuse = max(dot(normal, normalize(lightPos - point)), 0.0);
    }

    vec3 baseColor = vec3(0.4, 0.6, 0.8); // sea color
    vec4 dynamicSeaColor = vec4(baseColor * diffuse, 1.0);

    // compute reflection on the water
    vec3 reflectionRay = reflect(viewDirection, normal);
    reflectionRay = (inverse(mat) * vec4(reflectionRay, 0.0f)).xyz;
    vec4 reflectionColor = calculateRGB(origin, reflectionRay);

    dynamicSeaColor = mix(dynamicSeaColor, reflectionColor, 0.5f);

    return dynamicSeaColor;
}

void main()
{
    vec4 color = vec4(0.0f);
    mat4 mat = mat4(1.0f);
    mat[3] = vec4(meshTrans, 1.0f);
    vec3 origin = (inverse(mat) * vec4(rayOrigin, 1.0f)).xyz;
    vec3 direction = (inverse(mat) * vec4(rayDirection, 0.0f)).xyz;
    vec2 ret = intersectionKDTree(origin, direction);
    // vec2 ret = intersectionKDTree(rayOrigin, rayDirection);
    int idx = int(round(ret.x));
    float t = ret.y;
    vec4 dynamicSeaColor = renderDynamicSea(rayOrigin,  rayOrigin + rayDirection * 1000, t);
    if (idx < 0) {  // no intersection with mesh
        outColor = mix(vec4(0.529f, 0.808f, 0.922f, 1.0f), dynamicSeaColor, dynamicSeaColor.a);
        outDistance = -1.0f;
    }
    else {
        vec3 boxMin = vec3(-sea_width, sea_bottom, -sea_width);
        vec3 boxMax = vec3(sea_width, sea_top, sea_width);
        float tnear = intersectionAABB(boxMin, boxMax, rayOrigin, rayDirection).x;
        if (t < tnear) {
            mesh m = getMesh(idx);
            vec3 worldPosition = rayOrigin + t * rayDirection;
            color = vec4(m.diffuseColor * max(dot(normalize(lightPos - worldPosition), m.faceNormal), 0.0f), 1.0f);
            outColor = color;
            outDistance = t;
        }
        else {  // mesh is under water
            outColor = dynamicSeaColor;
            outDistance = tnear;
        }
    }
    // else {   // only intersection
    //     return vec4(1.0f);
    // }
}
