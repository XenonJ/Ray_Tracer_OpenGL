#version 330

#define baseBright  vec3(1.26,1.25,1.29)    // base color -- light
#define baseDark    vec3(0.31,0.31,0.32)    // base color -- dark
#define lightBright vec3(1.29, 1.17, 1.05)  // light color -- light
#define lightDark   vec3(0.7,0.75,0.8)      // light color -- dark

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
uniform sampler2D noiseTex; // noise texture to sample for cloud
uniform vec3 lightPos;  // light position in world space
uniform samplerBuffer worleyPoints; // Worley points texture for cloud

in vec3 pixelColor; // some background calculated by pixel(i, j)
in vec3 rayOrigin;  // camera position
in vec3 rayDirection;   // normalized direction for current ray

const float PI = 3.14159265359;
const float stepSize = 0.1;
const int MAX_STACK_SIZE = 1000;

// cloud box
#define bottom 5
#define top 15
#define width 20

out vec4 outputColor;

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

float noise(vec3 x)
{
    vec3 p = floor(x);
    vec3 f = fract(x);
    f = smoothstep(0.0, 1.0, f);
     
    vec2 uv = (p.xy+vec2(37.0, 17.0)*p.z) + f.xy;
    float v1 = texture( noiseTex, (uv)/256.0 ).x;
    float v2 = texture( noiseTex, (uv + vec2(37.0, 17.0))/256.0 ).x;
    return mix(v1, v2, f.z);
}
 
float getCloudNoise(vec3 worldPos) {
    vec3 coord = worldPos;
    coord *= 0.2;
    float n  = noise(coord) * 0.5;   coord *= 3.0;
          n += noise(coord) * 0.25;  coord *= 3.01;
          n += noise(coord) * 0.125; coord *= 3.02;
          n += noise(coord) * 0.0625;
    return max(n - 0.5, 0.0) * (1.0 / (1.0 - 0.5));
}

float getDensity( vec3 pos) {

	// Density falloff
	float mid = (bottom + top) / 2.0;
    float h = top - bottom;
    float weight = 1.0 - 2.0 * abs(mid - pos.y) / h;
    weight = pow(weight, 0.5);
	// Get noise
	vec2 coord = pos.xz * 0.005;
	coord += frameCounter * 0.0001;
    float noise = getCloudNoise(pos);
	noise += texture(noiseTex, coord * 1.0).x / 1.0;
	noise += texture(noiseTex, coord * 6.0).x / 6.0;
	noise += texture(noiseTex, coord * 12.0).x / 12.0;

	noise = noise * weight;

	if(noise < 0.7) {
		noise = 0.0;
	}
    return noise;
}

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

vec4 renderCloud(vec3 cameraPosition, vec3 worldPosition) {
    vec3 viewDirection = normalize(worldPosition - cameraPosition);
    vec3 step = viewDirection * stepSize;
    vec4 colorSum = vec4(0);

    // Box boundaries
    vec3 boxMin = vec3(-width, bottom, -width);
    vec3 boxMax = vec3(width, top, width);

    float tnear = intersectionAABB(boxMin, boxMax, cameraPosition, viewDirection).x;
    if (abs(tnear - -1.0f) < 1e-8f) {
        return vec4(0.0f);
    }

    // Calculate intersection point
    vec3 point = cameraPosition + viewDirection * max(tnear, 0.0);
	
	float len1 = length(point - cameraPosition);     
	float len2 = length(worldPosition - cameraPosition); 
		if(len2<len1) {
			return vec4(0);
		}
    // Ray marching
    float baseStepSize = stepSize;
    float maxJitter = baseStepSize * 0.5;
    
    for (int i = 0; i < 1000; i++) {
        vec2 noiseCoord = (point.xz + vec2(frameCounter)) * 0.01;
        float jitter = (texture(noiseTex, noiseCoord).r * 2.0 - 1.0) * maxJitter;
        vec3 stepWithJitter = step * (1.0 + jitter + 0.01 * i);
        point += stepWithJitter;

        // Early exit
        if (point.x < boxMin.x || point.x > boxMax.x ||
            point.y < boxMin.y || point.y > boxMax.y ||
            point.z < boxMin.z || point.z > boxMax.z) {
            break;
        }

        float density = getDensity(point) ;
        vec3 L = normalize(lightPos - point);                       // 光源方向
        float lightDensity = getDensity(point + L);       // 向光源方向采样一次 获取密度
        float delta = clamp(density - lightDensity, 0.0, 1.0);      // 两次采样密度差

        // 控制透明度
        density *= 0.3;

        // 颜色计算
        vec3 base = mix(baseBright, baseDark, density) * density;   // 基础颜色
        vec3 light = mix(lightDark, lightBright, delta);            // 光照对颜色影响

        // 混合
        vec4 color = vec4(base*light, density);                     // 当前点的最终颜色
        colorSum = color * (1.0 - colorSum.a) + colorSum;           // 与累积的颜色混

        if (colorSum.a > 0.95) {
            break;
        }
    }

    return colorSum;
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

vec4 calculateRGB() {
    vec4 color = vec4(0.0f);
    // vec2 ret = intersection(rayOrigin, rayDirection);
    vec2 ret = intersectionKDTree(rayOrigin, rayDirection);
    int idx = int(round(ret.x));
    float t = ret.y;
    if (idx < 0) {
        return vec4(0.0f);
    }
    // else {   // only intersection
    //     return vec4(1.0f);
    // }
    mesh m = getMesh(idx);
    vec3 worldPosition = rayOrigin + t * rayDirection;
    color = vec4(m.diffuseColor * max(dot(normalize(lightPos - worldPosition), m.faceNormal), 0.0f), 1.0f);
    return color;
}

void main()
{
    // vec4 rtColor = calculateRGB();
    // outputColor = rtColor;
    // node root = getNode(rootIndex);
    // node left = getNode(root.left);
    // node right = getNode(root.right);
    // node leftleft = getNode(left.left);
    // node leftright = getNode(left.right);
    // vec2 ret = intersectionAABB(leftright.min_xyz, leftright.max_xyz, rayOrigin, rayDirection);
    // mesh m = getMesh(leftleft.meshes[0]);
    // float mret = intersectionTriangle(m, rayOrigin, rayDirection);
    // vec2 iret = intersectionKDTreeRec(rayOrigin, rayDirection, 0);
    // outputColor = vec4(mret, 1.0f, 1.0f, 1.0f);
    // vec3 somevec = rayOrigin + rayDirection * 1000;
    // vec4 newColor = renderCloud(rayOrigin, somevec);
    vec4 cloudColor = renderCloud(rayOrigin,  rayOrigin + rayDirection * 1000);
	outputColor = vec4(1.0f);
}
