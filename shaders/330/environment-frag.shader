#version 330

#define baseBright  vec3(1.26,1.25,1.29)    // base color -- light
#define baseDark    vec3(0.31,0.31,0.32)    // base color -- dark
#define lightBright vec3(1.29, 1.17, 1.05)  // light color -- light
#define lightDark   vec3(0.7,0.75,0.8)      // light color -- dark

uniform int frameCounter;   // incr per frame
uniform sampler2D noiseTex; // noise texture to sample for cloud
uniform vec3 lightPos;  // light position in world space
uniform samplerBuffer worleyPoints; // Worley points texture for cloud
// output from ray tracing
uniform sampler2D colorMap;
uniform sampler2D distanceMap;

in vec3 pixelColor; // some background calculated by pixel(i, j)
in vec3 rayOrigin;  // camera position
in vec3 rayDirection;   // normalized direction for current ray
in vec2 pixelCoords;    // pixel coords

const float PI = 3.14159265359;
const float stepSize = 0.25;
const int MAX_STACK_SIZE = 1000;

// cloud box
#define bottom -10
#define top 10
#define width 20

out vec4 outputColor;

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
    
    for (int i = 0; i < 100; i++) {
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

void main()
{
    vec4 color = texture(colorMap, pixelCoords);
	// outputColor = vec4(color.rgb, 1.0);
	vec4 cloudColor = renderCloud(rayOrigin,  rayOrigin + rayDirection * 1000);
	outputColor = mix(vec4(color.rgb, 1.0f), cloudColor, cloudColor.a);
	// outputColor = vec4(pixelColor, 1.0f	);
}