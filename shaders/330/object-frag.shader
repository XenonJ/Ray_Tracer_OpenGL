#version 330

uniform samplerBuffer meshTexture;
uniform float meshSize;

in vec3 pixelColor;
in vec3 rayOrigin;
in vec3 rayDirection;
in vec3 lightDirection;

const float PI = 3.14159265359;
const float stepSize = 0.25;
vec3 worldPosition = rayOrigin + rayDirection * 10000;
uniform sampler2D noiseTex;

#define bottom -0.5
#define top 0.5
#define width 80

out vec4 outputColor;

float getDensity(sampler2D noisetex, vec3 pos) {
    vec2 coord = pos.xz * 0.0025;
    float noise = texture(noisetex, coord).x;
    return noise;
}

struct mesh {
    vec3 v1, v2, v3;
    vec3 n1, n2, n3;
    vec3 diffuseColor;
    vec3 type;
};

vec4 renderCloud(vec3 cameraPosition, vec3 worldPosition) {
    vec3 viewDirection = normalize(worldPosition - cameraPosition);
    vec3 step = viewDirection * stepSize;
    vec4 colorSum = vec4(0);

    // Box boundaries
    vec3 boxMin = vec3(-width, bottom, -width);
    vec3 boxMax = vec3(width, top, width);

    // Initialize tnear and tfar
    float tnear = -1e10;
    float tfar = 1e10;

    // Check each axis
    for (int i = 0; i < 3; i++) {
        if (viewDirection[i] != 0.0) {
            float t1 = (boxMin[i] - cameraPosition[i]) / viewDirection[i];
            float t2 = (boxMax[i] - cameraPosition[i]) / viewDirection[i];

            if (t1 > t2) {
                float temp = t1;
                t1 = t2;
                t2 = temp;
            }

            tnear = max(tnear, t1);
            tfar = min(tfar, t2);

            if (tnear > tfar || tfar < 0.0) {
                return vec4(0); // No intersection
            }
        } else {
            // Ray is parallel to the slabs
            if (cameraPosition[i] < boxMin[i] || cameraPosition[i] > boxMax[i]) {
                return vec4(0); // Ray misses the box
            }
        }
    }

	
    // Calculate intersection point
    vec3 point = cameraPosition + viewDirection * max(tnear, 0.0);
	
	float len1 = length(point - cameraPosition);     
	float len2 = length(worldPosition - cameraPosition); 
		if(len2<len1) {
			return vec4(0);
		}
    // Ray marching
    for (int i = 0; i < 10000; i++) {
        point += step;
		// Early exit
        if (point.x < boxMin.x || point.x > boxMax.x ||
            point.y < boxMin.y || point.y > boxMax.y ||
            point.z < boxMin.z || point.z > boxMax.z) {
            break;
        }

        float density = getDensity(noiseTex, point) * 0.2;
        vec4 color = vec4(1.0, 1.0, 1.0, 1.0) * density;
        colorSum = colorSum + color * (1.0 - colorSum.a);

        if (colorSum.a > 0.95) {
            break;
        }
    }

    return colorSum;
}

mesh getMesh(int index) {
    mesh ret;
    ret.v1 = texelFetch(meshTexture, 8 * index).rgb;
    ret.v2 = texelFetch(meshTexture, 8 * index + 1).rgb;
    ret.v3 = texelFetch(meshTexture, 8 * index + 2).rgb;
    ret.n1 = texelFetch(meshTexture, 8 * index + 3).rgb;
    ret.n2 = texelFetch(meshTexture, 8 * index + 4).rgb;
    ret.n3 = texelFetch(meshTexture, 8 * index + 5).rgb;
    ret.diffuseColor = texelFetch(meshTexture, 8 * index + 6).rgb;
    ret.type = texelFetch(meshTexture, 8 * index + 7).rgb;
    return ret;
}

float intersectionTriangle(mesh m, vec3 origin, vec3 direction) {
    vec3 e1 = m.v2 - m.v1;
    vec3 e2 = m.v3 - m.v1;
    vec3 pvec = cross(direction, e2);
    float det = dot(e1, pvec);
    if (abs(det) < 1e-8f) {
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

int intersection() {
    int idx = -1;
    float t = -1.0f;
    for (int i = 0; i < meshSize; i++) {
        mesh m = getMesh(i);
        float tmpt = intersectionTriangle(m, rayOrigin, rayDirection);
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
    return idx;
}

vec4 calculateRGB() {
    vec4 color;
    int idx = intersection();
    if (idx < 0) {
        return vec4(0.0f);
    }
    // else {
    //     return vec4(1.0f);
    // }
    mesh m = getMesh(idx);
    vec3 normal = (m.n1 + m.n2 + m.n3) / 3;
    color = vec4(m.diffuseColor * dot(lightDirection, normal), 1.0f);
    return color;
}

void main()
{
    outputColor = calculateRGB();
    // outputColor = vec4(meshSize / 200, 1.0f, 1.0f, 1.0f);
	// outputColor = mix(vec4(pixelColor, 1.0), renderCloud(rayOrigin,  rayOrigin + rayDirection * 1000), 0.5);
}
