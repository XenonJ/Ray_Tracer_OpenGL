#version 330

in vec3 pixelColor;
in vec3 rayOrigin;
in vec3 rayDirection;

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

void main() {
    outputColor = mix(vec4(pixelColor, 1.0), renderCloud(rayOrigin, rayOrigin + rayDirection * 1000), 0.5);
}
