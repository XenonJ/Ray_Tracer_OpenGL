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
#define bottom 1
#define top 10
#define width 250

out vec4 outputColor;

// sea box
#define sea_bottom -5
#define sea_top -4
#define sea_width 200

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
// 在 getCloudNoise 函数中添加基于距离的细节控制
float getCloudNoise(vec3 worldPos, float distanceFromCamera) {
    vec3 coord = worldPos * 0.005;
    coord.x += frameCounter * 0.0002;
    coord.z += frameCounter * 0.0002;
    coord.y -= frameCounter * 0.0002;

    
    // 基于距离计算细节层级
    float detailFactor = 1.0 - smoothstep(0.0, 1000.0, distanceFromCamera);
    
    // 基础噪声层
    float n = noise(coord) * 0.55;
    
    // 根据距离动态添加细节层
    if(detailFactor > 0.3) {
        coord *= 3.0;
        n += noise(coord) * 0.25 * detailFactor;
        
        if(detailFactor > 0.6) {
            coord *= 3.01;
            n += noise(coord) * 0.125 * detailFactor;
            
            if(detailFactor > 0.8) {
                coord *= 3.02;
                n += noise(coord) * 0.0625 * detailFactor;
            }
        }
    }
    
    // Control the threshold of the noise
    float threshold = 0.45;
    
    return max(n - threshold, 0.0) * (1.0 / (1.0 - threshold));
}



// Compute Density at a given position
float getDensity(vec3 pos, float distanceFromCamera) {
    vec3 boxMin = vec3(-width, bottom, -width);
    vec3 boxMax = vec3(width, top, width);
    
    // 增加边界过渡区域的宽度
    float transitionWidth = width * 0.3;
    float transitionHeight = (top - bottom) * 0.5;
    
    // 计算到边界的距离
    float distToEdgeX = min(abs(pos.x - boxMin.x), abs(pos.x - boxMax.x));
    float distToEdgeZ = min(abs(pos.z - boxMin.z), abs(pos.z - boxMax.z));
    float distToEdgeY = min(abs(pos.y - boxMin.y), abs(pos.y - boxMax.y));
    
    // 计算边界权重
    float horizontalEdgeFade = min(distToEdgeX, distToEdgeZ);
    float horizontalWeight = smoothstep(0.0, transitionWidth, horizontalEdgeFade);

    float verticalWeight = smoothstep(0.0, transitionHeight, distToEdgeY);
    
    float edgeWeight = pow(verticalWeight, 4);
    
    // 原有的高度权重计算
    float mid = (bottom + top) / 2.0;
    float h = top - bottom;
    float heightWeight = 1.0 - 2.0 * abs(mid - pos.y) / h;
    heightWeight = pow(heightWeight, 0.25);
    heightWeight = smoothstep(0.0, 1.0, heightWeight);
    
    // 获取基础噪声
    float noise = getCloudNoise(pos, distanceFromCamera);
    
    // 在边界附近削弱噪声
    noise *= edgeWeight;
    
    float density = noise ;
    
    // 提高密度阈值
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
vec4 renderCloud(vec3 cameraPosition, vec3 worldPosition, float depth) {
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
    float distanceFromCamera = length(point - cameraPosition);

    float len1 = length(point - cameraPosition);     
    float len2 = length(worldPosition - cameraPosition); 
    if(len2<len1) {
        return vec4(0);
    }

    // Ray marching
    float baseStepSize = stepSize;
    float maxJitter = baseStepSize * 0.5;
    float accumulatedDensity = 0.0;
    
    // 根据距离调整采样步长
    float adaptiveStepSize = stepSize * (1.0 + smoothstep(0.0, 100.0, distanceFromCamera) * 2.0);
    
    for (int i = 0; i < 200; i++) {
        vec2 noiseCoord = (point.xz + vec2(frameCounter)) * 0.01;
        float jitter = generateStepJitter(point, 0.1) * 0.3;
        
        // 使用自适应步长
        vec3 stepWithJitter = viewDirection * (adaptiveStepSize * (1.0 + jitter));
        point += stepWithJitter;

        if (point.x < boxMin.x || point.x > boxMax.x ||
            point.y < boxMin.y || point.y > boxMax.y ||
            point.z < boxMin.z || point.z > boxMax.z) {
            break;
        }

        if (depth > 0.0f && 
            ((point.x - cameraPosition.x) / viewDirection.x > depth || 
            (point.y - cameraPosition.y) / viewDirection.y > depth || 
            (point.z - cameraPosition.z) / viewDirection.z > depth)) {
            break;
        }

        float currentDistance = length(point - cameraPosition);
        float density = getDensity(point, currentDistance);

        density *= 1.5;
        vec3 L = normalize(lightPos - point);
        
        // 根据距离调整光照采样
        float lightSampleDist = 5.0 + currentDistance * 0.1;
        float lightDensity = getDensity(point + L * lightSampleDist, currentDistance);
        float delta = clamp(density - lightDensity, 0.0, 1.0);

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

void main()
{
    vec4 color = vec4(texture(colorMap, pixelCoords).rgb, 1.0f);
    float depth = texture(distanceMap, pixelCoords).r;
	vec4 cloudColor = renderCloud(rayOrigin,  rayOrigin + rayDirection * 1000, depth);
    color = mix(color, cloudColor, cloudColor.a);
    outputColor = color;
}
