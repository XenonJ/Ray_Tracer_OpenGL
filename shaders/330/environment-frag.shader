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
    1.262388, 6.022556, 0.563287, 0.491609,
    3.707309, 0.957211, 5.026175, 4.688653,
    0.073944, 5.390519, 1.149298, 2.267692,
    0.192578, 0.3774, 4.142232, 3.405905,
    5.352445, 4.526648, 2.365272, 6.094538,
    2.511331, 1.689678, 1.713087, 1.589109,
    4.797827, 1.954108, 2.532385, 5.31451,
    4.599562, 6.02215, 5.351632, 4.341203,
    1.471952, 0.498261, 3.497318, 3.371356,
    4.06391, 4.837089, 4.395204, 3.248211,
    1.285161, 3.37646, 1.772799, 4.383327,
    2.28784, 0.877712, 3.839675, 0.006322,
    1.638648, 4.17666, 3.983195, 2.359698,
    4.495201, 1.466667, 1.574463, 3.049083,
    0.788105, 1.627937, 1.417166, 5.326704,
    2.932744, 5.010021, 0.155426, 2.93011,
    1.935975, 1.017987, 5.901717, 3.425859,
    1.80488, 4.710533, 5.163953, 2.393426,
    5.625861, 1.162996, 2.124568, 4.167777,
    6.223719, 0.887796, 0.503856, 0.694372,
    2.16192, 3.667814, 5.645252, 4.685073,
    6.152803, 0.760109, 1.287305, 5.718012,
    3.339247, 1.580585, 3.072731, 2.119589,
    2.692128, 1.479551, 1.45894, 5.245956,
    3.233173, 3.397348, 3.336386, 4.937089,
    5.854844, 5.40838, 1.891949, 3.142829,
    1.114095, 2.97293, 4.834891, 5.222093,
    0.893334, 1.874481, 3.96831, 0.853536,
    5.373985, 4.420437, 3.855453, 2.805257,
    5.898182, 5.517356, 4.939854, 0.417501,
    6.218127, 4.11453, 3.698384, 2.739425,
    3.588778, 1.297301, 2.327105, 2.638228,
    0.407998, 3.026009, 2.091351, 1.099544,
    2.665816, 5.812187, 4.979064, 2.959433,
    2.851723, 4.924155, 6.8e-05, 0.126441,
    6.059776, 1.241011, 5.03934, 0.31538,
    5.425539, 3.51527, 4.97561, 1.983631,
    1.521594, 2.413134, 3.192946, 5.026141,
    4.330949, 6.091165, 0.726941, 4.185017,
    2.846085, 2.534428, 2.960805, 5.91905,
    5.860553, 1.526485, 5.32122, 1.216114,
    2.16261, 2.670073, 3.3733, 0.228404,
    2.473413, 3.598007, 5.310584, 5.615656,
    0.733727, 4.674816, 0.150943, 5.341097,
    3.748464, 5.959311, 2.738856, 2.569939,
    1.815576, 4.790562, 2.419276, 0.59787,
    1.31942, 5.13044, 1.291099, 0.390996,
    5.972384, 2.632814, 4.330092, 3.377545,
    0.997801, 0.590778, 5.288873, 1.359232,
    4.007544, 4.414426, 5.630541, 3.661653,
    5.406367, 5.657567, 1.732553, 0.281326,
    1.318062, 1.884988, 3.70938, 3.242056,
    1.802464, 2.380864, 0.225253, 2.828077,
    2.068613, 5.688474, 4.009067, 4.350369,
    5.768898, 1.793788, 5.196934
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
