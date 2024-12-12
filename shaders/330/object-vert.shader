#version 330 core

// const
const float PI = 3.14159265359;

// Uniform
uniform vec3 eyePosition;
uniform vec3 lookVec;
uniform vec3 upVec;
uniform float viewAngle;
uniform float nearPlane;
uniform float screenWidth;
uniform float screenHeight;
uniform vec3 lightPos;

// index
in vec2 pixelIndex; // [0, screenWidth-1] and [0, screenHeight-1]

// outputs
out vec3 rayOrigin;     // ray origin (eye position)
out vec3 rayDirection;  // ray dir
out vec3 pixelColor;
out vec3 lightDirection;

void main() {
	float ndcX = (pixelIndex.x / screenWidth) * 2.0 - 1.0;
    float ndcY = (pixelIndex.y / screenHeight) * 2.0 - 1.0;
	gl_Position = vec4(ndcX, ndcY, 0.0, 1.0); // screen location
	float r = (ndcX + 1.0) / 2.0;
	float g = (ndcY + 1.0) / 2.0;
	pixelColor = vec3(r, g, 1.0f);

    vec3 Q = eyePosition + lookVec * nearPlane;	// filmPosition
	float theta = viewAngle / 180.0f * PI;
	float H = nearPlane * tan(theta / 2.0f);
	float W = H * screenWidth / screenHeight;
	float nCols = screenWidth;
	float a = -W + 2 * W * (pixelIndex.x / nCols);
	float nRows = screenHeight;
	float b = -H + 2 * H * (pixelIndex.y / nRows);
	vec3 u = cross(lookVec, upVec);
	vec3 v = upVec;
	vec3 S = Q + a * u + b * v;
	// gl_Position = vec4(S, 1.0f);
	rayOrigin = eyePosition;
	rayDirection = normalize(S - eyePosition);
	lightDirection = normalize(lightPos - S);
}

// cpp version
// glm::vec3 generateRay(glm::vec3 eyePosition, int pixelX, int pixelY, Camera* camera) {
// 	glm::vec3 Q = eyePosition + camera->getLookVector() * NEAR_PLANE; // filmPosition
// 	float theta = camera->getViewAngle() / 180.0f * PI;
// 	float H = NEAR_PLANE * tan(theta / 2.0);
// 	float W = H * camera->getScreenWidth() / camera->getScreenHeight();
// 	float nCols = camera->getScreenWidth();
// 	float a = -W + 2 * W * (pixelX / nCols);
// 	float nRows = camera->getScreenHeight();
// 	float b = -H + 2 * H * (pixelY / nRows);
// 	glm::vec3 u = glm::cross(camera->getLookVector(), camera->getUpVector());
// 	glm::vec3 v = camera->getUpVector();
// 	glm::vec3 S = Q + a * u + b * v;
// 	return glm::normalize(S - eyePosition);
// }

