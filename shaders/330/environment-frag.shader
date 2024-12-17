#version 330

#define baseBright  vec3(1.26,1.25,1.29)    // base color -- light
#define baseDark    vec3(0.31,0.31,0.32)    // base color -- dark
#define lightBright vec3(1.29, 1.17, 1.05)  // light color -- light
#define lightDark   vec3(0.7,0.75,0.8)      // light color -- dark

uniform int frameCounter;   // incr per frame
uniform sampler3D noiseTex; // noise texture to sample for cloud
uniform sampler2D seaTex;
uniform sampler2D seaNormalTex;
uniform vec3 lightPos;  // light position in world space
uniform samplerBuffer worleyPoints; // Worley points texture for cloud
// output from ray tracing
uniform sampler2D colorMap;
uniform sampler2D distanceMap;

in vec3 pixelColor; // some background calculated by pixel(i, j)
in vec3 rayOrigin;  // camera position
in vec3 rayDirection;   // normalized direction for current ray
in vec2 pixelCoords;    // pixel coords

const float stepSize = 0.1;   // step size for ray marching
const int MAX_STACK_SIZE = 1000;

// cloud box
#define bottom -10
#define top 10
#define width 2

out vec4 outputColor;

// sea box
#define sea_bottom -1
#define sea_top 0
#define sea_width 20

// Debug method for visualizing values
vec4 debug(float value, float minValue, float maxValue) {
    vec3 debugColor;

    if (value > maxValue) {
        debugColor = vec3(1.0, 0.0, 0.0);// Too large: Red
    } else if (value < minValue) {
        debugColor = vec3(0.0, 0.0, 1.0);// Too small: Blue
    } else {
        // Map value between minValue (Black) and maxValue (White)
        float normalizedValue = (value - minValue) / (maxValue - minValue);
        debugColor = vec3(normalizedValue); // Grayscale gradient
    }
    // Return the final debug color as vec4
    return vec4(debugColor, 1.0);
}


// 3D Noise Function
float noise(vec3 coord) {
    return texture(noiseTex, coord).x;  // Sample 3D texture
}

// Generate Cloud Noise based on world position
float getCloudNoise(vec3 worldPos) {
    vec3 coord = worldPos * 0.005;
	coord += frameCounter * 0.0002;
    float n = noise(coord) * 0.55;
    coord *= 3.0;
    n += noise(coord) * 0.25;
    coord *= 3.01;
    n += noise(coord) * 0.125;
    coord *= 3.02;
    n += noise(coord) * 0.0625;
    return max(n - 0.5, 0.0) * (1.0 / (1.0 - 0.5));
}

// Compute Density at a given position
float getDensity(vec3 pos) {
    // Density falloff
    float mid = (bottom + top) / 2.0;
    float h = top - bottom;
    float weight = 1.0 - 2.0 * abs(mid - pos.y) / h;
    weight = pow(weight, 0.5);

    // Noise-based density
    float noise = getCloudNoise(pos);
	float density = noise * weight;
	if(density < 0.01) {
		density = 0.0;
	}
    return density;
}

// AABB intersection for cloud box
vec2 intersectionAABB(vec3 boxMin, vec3 boxMax, vec3 origin, vec3 direction) {
    float tnear = -1e10;
    float tfar = 1e10;

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

// Generate smooth noise-based jitter
float generateStepJitter(vec3 point, float frequency) {
    return texture(noiseTex, point * frequency).r; // Sample 3D noise
}

// Cloud rendering function
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
    
    for (int i = 0; i < 200; i++) {
        vec2 noiseCoord = (point.xz + vec2(frameCounter)) * 0.01;
        float jitter = generateStepJitter(point, 0.1) * 0.3; // Noise-based jitter in [-0.3, 0.3]
        vec3 stepWithJitter = viewDirection * (stepSize * (1.0 + jitter));
        point += stepWithJitter;

        // Early exit
        if (point.x < boxMin.x || point.x > boxMax.x ||
            point.y < boxMin.y || point.y > boxMax.y ||
            point.z < boxMin.z || point.z > boxMax.z) {
            break;
        }

        float density = getDensity(point);
        vec3 L = normalize(lightPos - point);
        float lightDensity = getDensity(point + L);
        float delta = clamp(density - lightDensity, 0.0, 1.0);

		density *= 1.4;
        vec3 base = mix(baseBright, baseDark, density) * density;
        vec3 light = mix(lightDark, lightBright, delta);

        vec4 color = vec4(base * light, density);
        colorSum = color * (1.0 - colorSum.a) + colorSum;

        if (colorSum.a > 0.98) {
            break;
        }
    }
    return colorSum;
}

vec4 renderSea(vec3 cameraPosition, vec3 worldPosition)
{
    // vertex
    vec3 viewDirection = normalize(worldPosition - cameraPosition);
    vec3 boxMin = vec3(-sea_width, sea_bottom, -sea_width);
    vec3 boxMax = vec3(sea_width, sea_top, sea_width);
    float tnear = intersectionAABB(boxMin, boxMax, cameraPosition, viewDirection).x;
    if (abs(tnear - -1.0f) < 1e-8f) {
        return vec4(0.0f);
    }
    vec3 point = cameraPosition + viewDirection * max(tnear, 0.0);
    // map pic
    vec2 texCoord = 1 * vec2(point.x/sea_width, point.z/sea_width);// repeat
    vec2 scaledTexCoord = texCoord * 1;
    vec4 texColor = texture(seaTex, scaledTexCoord);
    // map normal
    vec3 normalizedNormal = vec3(0.0, 1.0, 0.0);
    float diffuseFromGeometry = max(dot(normalizedNormal, normalize(lightPos - point)), 0.0);
    vec3 normalMapValue = texture(seaNormalTex, scaledTexCoord).rgb;
    vec3 normalFromNormalMap = normalMapValue * 2.0 - 1.0;
    vec3 mapNormal = normalize(normalFromNormalMap);
    float diffuseFromNormalMap = max(dot(mapNormal, normalize(lightPos - point)), 0.0);
    float combinedDiffuse = diffuseFromGeometry * diffuseFromNormalMap;
    
    return texColor * combinedDiffuse;
}

const int freqCount = 64;

const vec2 freqDir[64] = vec2[](
    vec2(0.874, -0.485), vec2(-0.904, 0.427), vec2(-0.399, 0.917), vec2(0.997, -0.074),
    vec2(0.493, 0.87), vec2(-0.116, 0.993), vec2(-0.507, 0.862), vec2(0.997, 0.084),
    vec2(-0.773, 0.635), vec2(-0.936, -0.352), vec2(-0.917, 0.399), vec2(0.94, -0.341),
    vec2(0.564, -0.826), vec2(0.977, -0.211), vec2(0.824, -0.567), vec2(0.753, 0.658),
    vec2(0.924, -0.383), vec2(0.684, 0.73), vec2(0.708, 0.706), vec2(0.689, -0.724),
    vec2(-0.564, 0.826), vec2(-0.995, 0.096), vec2(-0.873, -0.489), vec2(0.234, -0.972),
    vec2(-0.353, -0.936), vec2(-0.311, 0.95), vec2(0.219, -0.976), vec2(-0.011, 1.0),
    vec2(-0.715, 0.699), vec2(-0.886, 0.464), vec2(-0.937, 0.349), vec2(0.82, -0.573),
    vec2(-0.437, -0.9), vec2(-0.467, -0.884), vec2(0.998, 0.067), vec2(-0.229, -0.973),
    vec2(0.712, 0.702), vec2(0.505, 0.863), vec2(0.7, 0.714), vec2(0.904, 0.428),
    vec2(0.28, 0.96), vec2(-1.0, 0.029), vec2(0.661, 0.75), vec2(-0.96, 0.28),
    vec2(-0.42, -0.907), vec2(0.949, 0.314), vec2(0.185, 0.983), vec2(-0.368, -0.93),
    vec2(-0.912, -0.411), vec2(0.226, 0.974), vec2(0.162, -0.987), vec2(0.993, -0.118),
    vec2(-0.689, -0.725), vec2(0.623, 0.782), vec2(0.493, -0.87), vec2(-0.976, -0.218),
    vec2(0.814, 0.581), vec2(-0.359, -0.933), vec2(0.07, -0.998), vec2(-0.033, 0.999),
    vec2(0.748, 0.663), vec2(0.278, -0.96), vec2(0.608, -0.794), vec2(-0.606, 0.795)
);

const float freqOmega[64] = float[](
    1.367, 1.33, 1.063, 1.204,
    1.447, 0.791, 1.142, 0.799,
    0.991, 0.628, 1.186, 1.347,
    1.342, 1.174, 1.32, 0.697,
    1.208, 1.347, 0.752, 0.5,
    1.408, 1.024, 0.592, 0.582,
    0.595, 0.85, 0.996, 1.124,
    1.18, 1.063, 0.937, 0.839,
    1.246, 0.596, 1.413, 1.046,
    0.576, 0.686, 0.819, 1.454,
    0.829, 0.707, 1.203, 1.4,
    0.927, 0.575, 0.663, 1.173,
    1.383, 1.314, 1.317, 0.815,
    0.729, 0.845, 1.393, 0.548,
    1.137, 1.007, 1.034, 1.009,
    0.903, 0.917, 0.964, 0.778
);

const float freqAmp[64] = float[](
    0.028, 0.03, 0.016, 0.024,
    0.021, 0.039, 0.019, 0.028,
    0.02, 0.016, 0.047, 0.036,
    0.013, 0.017, 0.055, 0.028,
    0.052, 0.041, 0.02, 0.03,
    0.06, 0.033, 0.054, 0.028,
    0.022, 0.057, 0.019, 0.017,
    0.036, 0.027, 0.052, 0.058,
    0.026, 0.04, 0.057, 0.033,
    0.039, 0.042, 0.041, 0.021,
    0.016, 0.019, 0.011, 0.056,
    0.021, 0.051, 0.048, 0.037,
    0.054, 0.037, 0.017, 0.04,
    0.05, 0.018, 0.037, 0.048,
    0.033, 0.045, 0.045, 0.023,
    0.052, 0.054, 0.022, 0.057
);

const float freqPhaseOffset[64] = float[](
    1.134, 1.543, 0.313, 0.512,
    1.432, 1.488, 0.431, 0.652,
    1.382, 2.256, 2.407, 0.885,
    0.993, 1.924, 0.39, 1.789,
    1.013, 1.201, 2.402, 0.795,
    0.024, 0.369, 1.355, 2.439,
    0.306, 2.352, 2.346, 0.788,
    1.07, 2.283, 1.775, 0.05,
    2.125, 2.488, 0.075, 1.859,
    2.479, 2.279, 2.062, 0.611,
    2.28, 2.226, 2.221, 1.784,
    0.689, 0.731, 0.351, 0.275,
    2.432, 0.933, 1.489, 2.387,
    1.954, 0.086, 1.091, 1.73,
    0.116, 1.913, 2.319, 0.746,
    0.457, 0.248, 2.11, 2.167
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

// getting normal by partial derivative
void computeNormalFromHeight(vec2 pos, float t, out float H, out vec3 N) {
    float epsilon = 0.001;
    float H0 = computeHeightFromIFFT(pos, t);
    float Hx = (computeHeightFromIFFT(pos + vec2(epsilon,0), t) - computeHeightFromIFFT(pos - vec2(epsilon,0), t)) / (2.0*epsilon);
    float Hz = (computeHeightFromIFFT(pos + vec2(0,epsilon), t) - computeHeightFromIFFT(pos - vec2(0,epsilon), t)) / (2.0*epsilon);
    H = H0;
    N = normalize(vec3(-Hx, 1.0, -Hz));
}

vec4 renderDynamicSea(vec3 cameraPosition, vec3 worldPosition)
{
    vec3 viewDirection = normalize(worldPosition - cameraPosition);
    vec3 boxMin = vec3(-sea_width, sea_bottom, -sea_width);
    vec3 boxMax = vec3(sea_width, sea_top, sea_width);
    float tnear = intersectionAABB(boxMin, boxMax, cameraPosition, viewDirection).x;
    if (abs(tnear - -1.0f) < 1e-8f) {
        return vec4(0.0f);
    }
    vec3 point = cameraPosition + viewDirection * max(tnear, 0.0);

    vec2 pos = vec2(point.x, point.z);
    float time = float(frameCounter)*0.05; // dynamic time

    float H;
    vec3 normal;
    computeNormalFromHeight(pos, time, H, normal);

    float diffuse = max(dot(normal, normalize(lightPos - point)), 0.0);
    vec3 baseColor = vec3(0.0, 0.2, 0.4); // sea color
    vec4 dynamicSeaColor = vec4(baseColor * diffuse, 1.0);

    return dynamicSeaColor;
}



void main()
{
    vec4 color = texture(colorMap, pixelCoords);
	// outputColor = vec4(color.rgb, 1.0);
	vec4 cloudColor = renderCloud(rayOrigin,  rayOrigin + rayDirection * 1000);
	outputColor = mix(vec4(color.rgb, 1.0f), cloudColor, cloudColor.a);
	// outputColor = vec4(pixelColor, 1.0f	);

    vec4 seaColor = renderSea(rayOrigin,  rayOrigin + rayDirection * 1000);
    vec4 dynamicSeaColor = renderDynamicSea(rayOrigin,  rayOrigin + rayDirection * 1000);
    outputColor = mix(dynamicSeaColor, vec4(cloudColor.rgb, 1.0), cloudColor.a);
}
