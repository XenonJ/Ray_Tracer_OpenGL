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

// ocean IFFT (simplified simulation)
const int freqCount = 219;

const vec2 freqDir[219] = vec2[](
    vec2(0.298275, -0.95448), vec2(0.351123, -0.936329), vec2(0.400819, -0.916157), vec2(0.447214, -0.894427),
    vec2(0.490261, -0.871576), vec2(0.529999, -0.847998), vec2(0.566529, -0.824042), vec2(0.6, -0.8),
    vec2(0.630593, -0.776114), vec2(0.658505, -0.752577), vec2(0.683941, -0.729537), vec2(0.257663, -0.966235),
    vec2(0.316228, -0.948683), vec2(0.371391, -0.928477), vec2(0.422885, -0.906183), vec2(0.470588, -0.882353),
    vec2(0.514496, -0.857493), vec2(0.5547, -0.83205), vec2(0.591364, -0.806405), vec2(0.624695, -0.780869),
    vec2(0.654931, -0.755689), vec2(0.682318, -0.731055), vec2(0.707107, -0.707107), vec2(0.274721, -0.961524),
    vec2(0.336336, -0.941742), vec2(0.393919, -0.919145), vec2(0.447214, -0.894427), vec2(0.496139, -0.868243),
    vec2(0.540758, -0.841178), vec2(0.581238, -0.813733), vec2(0.617822, -0.786318), vec2(0.650791, -0.759257),
    vec2(0.680451, -0.732793), vec2(0.707107, -0.707107), vec2(0.731055, -0.682318), vec2(0.294086, -0.955779),
    vec2(0.358979, -0.933346), vec2(0.419058, -0.907959), vec2(0.4741, -0.880471), vec2(0.524097, -0.851658),
    vec2(0.56921, -0.822192), vec2(0.609711, -0.792624), vec2(0.645942, -0.763386), vec2(0.67828, -0.734803),
    vec2(0.707107, -0.707107), vec2(0.732793, -0.680451), vec2(0.755689, -0.654931), vec2(0.242536, -0.970143),
    vec2(0.316228, -0.948683), vec2(0.384615, -0.923077), vec2(0.447214, -0.894427), vec2(0.503871, -0.863779),
    vec2(0.5547, -0.83205), vec2(0.6, -0.8), vec2(0.640184, -0.768221), vec2(0.675725, -0.737154),
    vec2(0.707107, -0.707107), vec2(0.734803, -0.67828), vec2(0.759257, -0.650791), vec2(0.780869, -0.624695),
    vec2(0.263117, -0.964764), vec2(0.341743, -0.939793), vec2(0.413803, -0.910366), vec2(0.478852, -0.877896),
    vec2(0.536875, -0.843661), vec2(0.588172, -0.808736), vec2(0.633238, -0.773957), vec2(0.672673, -0.73994),
    vec2(0.707107, -0.707107), vec2(0.737154, -0.675725), vec2(0.763386, -0.645942), vec2(0.786318, -0.617822),
    vec2(0.806405, -0.591364), vec2(0.196116, -0.980581), vec2(0.287348, -0.957826), vec2(0.371391, -0.928477),
    vec2(0.447214, -0.894427), vec2(0.514496, -0.857493), vec2(0.573462, -0.819232), vec2(0.624695, -0.780869),
    vec2(0.668965, -0.743294), vec2(0.707107, -0.707107), vec2(0.73994, -0.672673), vec2(0.768221, -0.640184),
    vec2(0.792624, -0.609711), vec2(0.813733, -0.581238), vec2(0.83205, -0.5547), vec2(0.21693, -0.976187),
    vec2(0.316228, -0.948683), vec2(0.406138, -0.913812), vec2(0.485643, -0.874157), vec2(0.5547, -0.83205),
    vec2(0.613941, -0.789352), vec2(0.664364, -0.747409), vec2(0.707107, -0.707107), vec2(0.743294, -0.668965),
    vec2(0.773957, -0.633238), vec2(0.8, -0.6), vec2(0.822192, -0.56921), vec2(0.841178, -0.540758),
    vec2(0.857493, -0.514496), vec2(0.242536, -0.970143), vec2(0.351123, -0.936329), vec2(0.447214, -0.894427),
    vec2(0.529999, -0.847998), vec2(0.6, -0.8), vec2(0.658505, -0.752577), vec2(0.707107, -0.707107),
    vec2(0.747409, -0.664364), vec2(0.780869, -0.624695), vec2(0.808736, -0.588172), vec2(0.83205, -0.5547),
    vec2(0.851658, -0.524097), vec2(0.868243, -0.496139), vec2(0.882353, -0.470588), vec2(0.274721, -0.961524),
    vec2(0.393919, -0.919145), vec2(0.496139, -0.868243), vec2(0.581238, -0.813733), vec2(0.650791, -0.759257),
    vec2(0.707107, -0.707107), vec2(0.752577, -0.658505), vec2(0.789352, -0.613941), vec2(0.819232, -0.573462),
    vec2(0.843661, -0.536875), vec2(0.863779, -0.503871), vec2(0.880471, -0.4741), vec2(0.894427, -0.447214),
    vec2(0.906183, -0.422885), vec2(0.164399, -0.986394), vec2(0.316228, -0.948683), vec2(0.447214, -0.894427),
    vec2(0.5547, -0.83205), vec2(0.640184, -0.768221), vec2(0.707107, -0.707107), vec2(0.759257, -0.650791),
    vec2(0.8, -0.6), vec2(0.83205, -0.5547), vec2(0.857493, -0.514496), vec2(0.877896, -0.478852),
    vec2(0.894427, -0.447214), vec2(0.907959, -0.419058), vec2(0.919145, -0.393919), vec2(0.928477, -0.371391),
    vec2(0.196116, -0.980581), vec2(0.371391, -0.928477), vec2(0.514496, -0.857493), vec2(0.624695, -0.780869),
    vec2(0.707107, -0.707107), vec2(0.768221, -0.640184), vec2(0.813733, -0.581238), vec2(0.847998, -0.529999),
    vec2(0.874157, -0.485643), vec2(0.894427, -0.447214), vec2(0.910366, -0.413803), vec2(0.923077, -0.384615),
    vec2(0.933346, -0.358979), vec2(0.941742, -0.336336), vec2(0.948683, -0.316228), vec2(0.242536, -0.970143),
    vec2(0.447214, -0.894427), vec2(0.6, -0.8), vec2(0.707107, -0.707107), vec2(0.780869, -0.624695),
    vec2(0.83205, -0.5547), vec2(0.868243, -0.496139), vec2(0.894427, -0.447214), vec2(0.913812, -0.406138),
    vec2(0.928477, -0.371391), vec2(0.939793, -0.341743), vec2(0.948683, -0.316228), vec2(0.955779, -0.294086),
    vec2(0.961524, -0.274721), vec2(0.966235, -0.257663), vec2(0.316228, -0.948683), vec2(0.5547, -0.83205),
    vec2(0.707107, -0.707107), vec2(0.8, -0.6), vec2(0.857493, -0.514496), vec2(0.894427, -0.447214),
    vec2(0.919145, -0.393919), vec2(0.936329, -0.351123), vec2(0.948683, -0.316228), vec2(0.957826, -0.287348),
    vec2(0.964764, -0.263117), vec2(0.970143, -0.242536), vec2(0.974391, -0.22486), vec2(0.977802, -0.209529),
    vec2(0.980581, -0.196116), vec2(0.447214, -0.894427), vec2(0.707107, -0.707107), vec2(0.83205, -0.5547),
    vec2(0.894427, -0.447214), vec2(0.928477, -0.371391), vec2(0.948683, -0.316228), vec2(0.961524, -0.274721),
    vec2(0.970143, -0.242536), vec2(0.976187, -0.21693), vec2(0.980581, -0.196116), vec2(0.98387, -0.178885),
    vec2(0.986394, -0.164399), vec2(0.988372, -0.152057), vec2(0.989949, -0.141421), vec2(0.991228, -0.132164),
    vec2(0.707107, -0.707107), vec2(0.894427, -0.447214), vec2(0.948683, -0.316228), vec2(0.970143, -0.242536),
    vec2(0.980581, -0.196116), vec2(0.986394, -0.164399), vec2(0.989949, -0.141421), vec2(0.992278, -0.124035),
    vec2(0.993884, -0.110432), vec2(0.995037, -0.099504), vec2(0.995893, -0.090536), vec2(0.996546, -0.083045),
    vec2(0.997054, -0.076696), vec2(0.997459, -0.071247), vec2(0.997785, -0.066519)
);

const float freqOmega[219] = float[](
    4.01801, 4.056768, 4.101185, 4.150706,
    4.204767, 4.26282, 4.32434, 4.388837,
    4.455862, 4.525005, 4.595902, 3.866684,
    3.902289, 3.944524, 3.99275, 4.046308,
    4.104543, 4.166825, 4.232563, 4.301214,
    4.372285, 4.44534, 4.519991, 3.744712,
    3.783838, 3.830068, 3.88263, 3.94074,
    4.003634, 4.07059, 4.140943, 4.214094,
    4.28951, 4.366726, 4.44534, 3.619323,
    3.662561, 3.71341, 3.770931, 3.834188,
    3.902289, 3.974409, 4.049803, 4.127818,
    4.207883, 4.28951, 4.372285, 3.451495,
    3.490314, 3.538393, 3.594617, 3.657832,
    3.726922, 3.800844, 3.878662, 3.959551,
    4.042803, 4.127818, 4.214094, 4.301214,
    3.313754, 3.357489, 3.411322, 3.473836,
    3.543616, 3.619323, 3.699749, 3.783838,
    3.870689, 3.959551, 4.049803, 4.140943,
    4.232563, 3.133955, 3.170962, 3.22069,
    3.281421, 3.351345, 3.428712, 3.511926,
    3.599599, 3.690557, 3.783838, 3.878662,
    3.974409, 4.07059, 4.166825, 2.979814,
    3.0227, 3.079835, 3.148915, 3.227609,
    3.313754, 3.405465, 3.50117, 3.599599,
    3.699749, 3.800844, 3.902289, 4.003634,
    4.104543, 2.818134, 2.868568, 2.934992,
    3.014269, 3.103376, 3.199662, 3.300935,
    3.405465, 3.511926, 3.619323, 3.726922,
    3.834188, 3.94074, 4.046308, 2.647911,
    2.708267, 2.786524, 2.878342, 2.979814,
    3.087742, 3.199662, 3.313754, 3.428712,
    3.543616, 3.657832, 3.770931, 3.88263,
    3.99275, 2.420387, 2.468024, 2.541778,
    2.635332, 2.742628, 2.858693, 2.979814,
    3.103376, 3.227609, 3.351345, 3.473836,
    3.594617, 3.71341, 3.830068, 3.944524,
    2.216041, 2.277372, 2.369759, 2.483307,
    2.609618, 2.742628, 2.878342, 3.014269,
    3.148915, 3.281421, 3.411322, 3.538393,
    3.662561, 3.783838, 3.902289, 1.992722,
    2.075353, 2.194418, 2.334113, 2.483307,
    2.635332, 2.786524, 2.934992, 3.079835,
    3.22069, 3.357489, 3.490314, 3.619323,
    3.744712, 3.866684, 1.745157, 1.863461,
    2.021401, 2.194418, 2.369759, 2.541778,
    2.708267, 2.868568, 3.0227, 3.170962,
    3.313754, 3.451495, 3.58459, 3.71341,
    3.838296, 1.467496, 1.650467, 1.863461,
    2.075353, 2.277372, 2.468024, 2.647911,
    2.818134, 2.979814, 3.133955, 3.281421,
    3.422945, 3.559147, 3.690557, 3.817625,
    1.167057, 1.467496, 1.745157, 1.992722,
    2.216041, 2.420387, 2.609618, 2.786524,
    2.953166, 3.111106, 3.261553, 3.405465,
    3.543616, 3.676639, 3.80506
);

const float freqAmp[219] = float[](
    0.000137, 0.000214, 0.000305, 0.000403,
    0.000504, 0.000602, 0.000694, 0.000776,
    0.000847, 0.000905, 0.000952, 0.000103,
    0.000183, 0.000284, 0.000399, 0.000521,
    0.000643, 0.000758, 0.000861, 0.000951,
    0.001025, 0.001083, 0.001126, 0.000142,
    0.00025, 0.000382, 0.000529, 0.00068,
    0.000826, 0.000958, 0.001074, 0.001168,
    0.001242, 0.001296, 0.001332, 0.0002,
    0.000346, 0.000521, 0.000709, 0.000896,
    0.001068, 0.001219, 0.001343, 0.001439,
    0.001508, 0.001552, 0.001574, 0.000136,
    0.000288, 0.00049, 0.000722, 0.000963,
    0.001191, 0.001392, 0.001558, 0.001685,
    0.001774, 0.00183, 0.001855, 0.001857,
    0.000204, 0.000424, 0.000707, 0.001018,
    0.001324, 0.001598, 0.001825, 0.001997,
    0.002117, 0.002187, 0.002217, 0.002213,
    0.002184, 0.000106, 0.000318, 0.000644,
    0.001043, 0.001459, 0.001843, 0.002162,
    0.002404, 0.002567, 0.002659, 0.002692,
    0.002678, 0.00263, 0.002557, 0.000175,
    0.000513, 0.001008, 0.001577, 0.002127,
    0.002594, 0.002945, 0.003175, 0.003297,
    0.003332, 0.0033, 0.003219, 0.003107,
    0.002975, 0.000307, 0.000867, 0.001634,
    0.002443, 0.003153, 0.003686, 0.004026,
    0.004193, 0.004223, 0.004155, 0.004019,
    0.003843, 0.003644, 0.003435, 0.000572,
    0.001542, 0.002747, 0.003878, 0.004737,
    0.005266, 0.005502, 0.005513, 0.005372,
    0.005137, 0.00485, 0.004542, 0.00423,
    0.003928, 0.000176, 0.001157, 0.002909,
    0.004802, 0.00629, 0.007177, 0.007521,
    0.007473, 0.007179, 0.006754, 0.006271,
    0.005778, 0.0053, 0.004852, 0.004438,
    0.000425, 0.002587, 0.005866, 0.008705,
    0.010347, 0.01087, 0.010642, 0.010006,
    0.009195, 0.008344, 0.007524, 0.006768,
    0.006087, 0.005481, 0.004946, 0.00123,
    0.006553, 0.012658, 0.016182, 0.017001,
    0.016208, 0.014724, 0.013068, 0.011484,
    0.010063, 0.008826, 0.007764, 0.006858,
    0.006084, 0.005423, 0.004635, 0.019244,
    0.028785, 0.030005, 0.027158, 0.023274,
    0.019583, 0.016436, 0.013854, 0.011761,
    0.010066, 0.008686, 0.007555, 0.00662,
    0.00584, 0.026216, 0.06478, 0.064948,
    0.052428, 0.040429, 0.03125, 0.02454,
    0.019631, 0.015986, 0.013229, 0.011106,
    0.009441, 0.008115, 0.007043, 0.006166,
    0.258738, 0.209726, 0.125154, 0.078716,
    0.053128, 0.037981, 0.028392, 0.021978,
    0.017491, 0.014236, 0.011803, 0.009938,
    0.008479, 0.007315, 0.006373
);

const float freqPhaseOffset[219] = float[](
    4.650754, 4.488091, 1.637114, 0.246093,
    5.141558, 4.561329, 1.555918, 2.946002,
    5.52552, 5.50789, 5.825584, 4.133282,
    2.862883, 0.170583, 3.582707, 3.56602,
    0.147405, 3.961665, 0.440812, 3.367621,
    0.173839, 6.280952, 0.095179, 1.65256,
    1.792554, 1.366079, 2.580235, 5.042116,
    0.495638, 0.469781, 0.114142, 0.915349,
    3.120174, 5.962099, 4.532871, 4.668968,
    1.168853, 1.668453, 2.212626, 5.72417,
    1.499394, 0.196708, 1.938939, 2.482821,
    3.302992, 5.718826, 5.891389, 4.092438,
    6.02585, 2.12726, 3.883231, 1.619127,
    0.530437, 5.941872, 4.765975, 1.000779,
    5.668514, 4.899557, 0.548735, 3.106542,
    3.431652, 5.467816, 6.073736, 5.376951,
    5.149592, 2.837587, 3.840909, 0.359522,
    1.196904, 3.833122, 0.436376, 1.283008,
    5.090006, 2.875454, 4.408606, 1.968006,
    0.856376, 3.808005, 3.159395, 2.449334,
    3.515277, 2.442409, 0.92686, 0.927035,
    0.489779, 5.566993, 5.156742, 3.299249,
    2.533524, 5.034142, 0.990144, 2.861449,
    2.456678, 4.058622, 4.890187, 5.604105,
    1.824152, 4.412936, 1.069356, 1.708575,
    6.257481, 0.051451, 5.39586, 5.606332,
    2.289569, 1.401706, 4.56277, 5.85828,
    1.866899, 2.82277, 5.513309, 4.547151,
    3.928345, 0.137153, 6.011322, 6.126467,
    0.302164, 1.610188, 3.581419, 0.808545,
    3.279533, 6.122939, 3.770558, 0.615373,
    5.686857, 5.480854, 4.170025, 3.100133,
    3.531048, 2.156641, 1.825689, 6.175931,
    1.716337, 4.523045, 0.925033, 1.506702,
    1.857024, 2.511105, 1.14841, 3.436909,
    4.654612, 6.010221, 3.954467, 4.298273,
    5.0638, 2.141612, 0.334214, 5.687681,
    5.584391, 0.190997, 3.920122, 2.508301,
    1.387104, 6.081133, 1.859913, 2.117154,
    2.574736, 2.78449, 1.417824, 4.916538,
    1.908754, 2.949178, 6.019088, 2.980235,
    5.54935, 3.565603, 1.239782, 6.169106,
    1.639291, 0.639188, 1.658635, 2.413559,
    5.572204, 1.95784, 4.208262, 1.937385,
    4.64895, 5.054333, 5.857117, 1.43812,
    1.625909, 5.544022, 5.200676, 3.214309,
    1.300075, 4.700846, 2.331167, 4.226992,
    4.603448, 4.381447, 0.444681, 2.715758,
    0.670407, 3.720664, 2.033178, 3.105226,
    1.322702, 2.973483, 6.191831, 0.495306,
    1.050007, 4.86904, 5.431784, 1.350748,
    4.30519, 6.241481, 3.249098, 6.039938,
    2.667442, 6.026186, 6.132529, 2.188447,
    4.445669, 1.855848, 5.074367, 3.984935,
    4.252102, 5.052911, 4.323248
);



float computeHeightFromIFFT(vec2 pos, float t) {
    float height = 0.0;
    // h(x) = sum_k (A_k * exp(i*(freqDir·x - omega_k*t + phase_k)))
    for (int i=0; i<freqCount; i++) {
        float phase = dot(freqDir[i], pos) * freqOmega[i] - freqOmega[i]*t + freqPhaseOffset[i];
        height += freqAmp[i]*sin(phase);
    }
    return height;
}

void computeNormalFromHeight(vec2 pos, float t, out float H, out vec3 N) {
    float epsilon = 0.001;
    float H0 = computeHeightFromIFFT(pos, t);
    float Hx = (computeHeightFromIFFT(pos + vec2(epsilon,0), t) - computeHeightFromIFFT(pos - vec2(epsilon,0), t)) / (2.0*epsilon);
    float Hz = (computeHeightFromIFFT(pos + vec2(0,epsilon), t) - computeHeightFromIFFT(pos - vec2(0,epsilon), t)) / (2.0*epsilon);
    H = H0;
    N = normalize(vec3(-Hx, 1.0, -Hz));
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
    if (idx < 0) {
        outColor = mix(vec4(0.529f, 0.808f, 0.922f, 1.0f), dynamicSeaColor, dynamicSeaColor.a);
        outDistance = -1.0f;
    }
    else {
        mesh m = getMesh(idx);
        vec3 worldPosition = rayOrigin + t * rayDirection;
        color = vec4(m.diffuseColor * max(dot(normalize(lightPos - worldPosition), m.faceNormal), 0.0f), 1.0f);
        outColor = color;
        outDistance = t;
    }
    // else {   // only intersection
    //     return vec4(1.0f);
    // }
}
