#include "scene/Camera.h"
#include <OpenGL/OpenGL.h>
#include <cstdio>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Camera::Camera() { reset(); }

Camera::~Camera() {}

// 重置了相机的基本参数
void Camera::reset() {
  orientLookAt(glm::vec3(0.0f, 0.0f, DEFAULT_FOCUS_LENGTH),
               glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  setViewAngle(VIEW_ANGLE);
  setNearPlane(NEAR_PLANE);
  setFarPlane(FAR_PLANE);
  screenWidth = screenHeight = 200;
  screenWidthRatio = 1.0f;
  rotU = rotV = rotW = 0;
}

// 控制相机在三个轴上的旋转，并保持累积旋转效果
// called by main.cpp as a part of the slider callback for controlling rotation
//  the reason for computing the diff is to make sure that we are only
//  incrementally rotating the camera
void Camera::setRotUVW(float u, float v, float w) {
  float diffU = u - rotU;
  float diffV = v - rotV;
  float diffW = w - rotW;
  rotateU(diffU);
  rotateV(diffV);
  rotateW(diffW);
  rotU = u;
  rotV = v;
  rotW = w;
}

void Camera::orientLookAt(glm::vec3 eyePoint, glm::vec3 lookatPoint,
                          glm::vec3 upVec) {
  orientLookVec(eyePoint, lookatPoint - eyePoint, upVec);
}

void Camera::orientLookVec(glm::vec3 eyePoint, glm::vec3 lookVec,
                           glm::vec3 upVec) {
  position = eyePoint;
  lookVector = glm::normalize(lookVec);
  glm::vec3 right = glm::normalize(glm::cross(lookVector, upVec)); // right = cross (lookVec, upVec)
  upVector = glm::normalize(glm::cross(right, lookVector));
}

/* The resulting scale matrix (scaleMat4) is:

| 1/w_half   0         0         0 |
| 0         1/h_half   0         0 |
| 0         0         1/farPlane 0 |
| 0         0         0          1 |

This matrix is used to scale objects from world space to the correct size in projection space, based on the field of view, aspect ratio, and far plane distance.
*/

glm::mat4 Camera::getScaleMatrix() {
  glm::mat4 scaleMat4(1.0);

  float viewAngle = glm::radians(getViewAngle()); 
  float nearPlane = getNearPlane();              
  float farPlane = getFarPlane();               
  int screenWidth = getScreenWidth();          
  int screenHeight = getScreenHeight();       

  if (screenWidth <= 0 || screenHeight <= 0) {
    std::cerr << "Error: Screen dimensions must be positive." << std::endl;
    screenWidth = (screenWidth <= 0) ? 1 : screenWidth;
    screenHeight = (screenHeight <= 0) ? 1 : screenHeight;
  }

  float aspectRatio = static_cast<float>(screenWidth) / static_cast<float>(screenHeight); // 计算宽高比

  
  if (viewAngle <= 0.0f || viewAngle >= glm::radians(180.0f)) { // 检查视角是否有效 
    std::cerr << "Error: Invalid view angle." << std::endl;
    viewAngle = glm::radians(45.0f);
  }

  if (nearPlane <= 0.0f || nearPlane >= farPlane) {  // 检查裁剪面是否有效
    std::cerr << "Error: Invalid near or far plane values." << std::endl;
    nearPlane = 0.1f;
    farPlane = 100.0f;
  }

  float h_half = glm::tan(viewAngle / 2) * farPlane; // 计算缩放矩阵
  float w_half = h_half * aspectRatio;
  scaleMat4[0][0] = 1 / w_half;
  scaleMat4[1][1] = 1 / h_half;
  scaleMat4[2][2] = 1 / farPlane;
  return scaleMat4;
}

/* The resulting inverse scale matrix (invScaleMat4) is:

| w_half   0        0         0 |
| 0        h_half   0         0 |
| 0        0        farPlane  0 |
| 0        0        0         1 |

This matrix is the inverse of the scale matrix. It scales objects back from projection space to world space. The scaling factors are based on the field of view, aspect ratio, and far plane distance.
*/
glm::mat4 Camera::getInverseScaleMatrix() {
  glm::mat4 invScaleMat4(1.0);
  float viewAngle = getViewAngle();
  float nearPlane = getNearPlane();
  float farPlane = getFarPlane();
  int screenWidth = getScreenWidth();
  int screenHeight = getScreenHeight();
  float aspectRatio =
      static_cast<float>(screenWidth) / static_cast<float>(screenHeight);

  float h_half = glm::tan(viewAngle / 2) * farPlane;
  float w_half = h_half * aspectRatio;
  invScaleMat4[0][0] = w_half;
  invScaleMat4[1][1] = h_half;
  invScaleMat4[2][2] = farPlane;
  return invScaleMat4;
}

/* The resulting unhinge matrix (unhingeMat4) is:

| 1    0    0    0      |
| 0    1    0    0      |
| 0    0   -(1/(c+1))  -1 |
| 0    0   c/(c+1)   0      |

Where c = -(nearPlane / farPlane).

This matrix is used to map the Z-depth values from world space to clip space for correct depth calculations.
*/
glm::mat4 Camera::getUnhingeMatrix() {
  glm::mat4 unhingeMat4(1.0);
  float nearPlane = getNearPlane(); 
  float farPlane = getFarPlane();  
  float c = -(nearPlane / farPlane);

  if (nearPlane <= 0.0f || nearPlane >= farPlane) {
    std::cerr << "Error: Invalid near or far plane values." << std::endl;
    nearPlane = 0.1f;
    farPlane = 100.0f;
  }
  unhingeMat4[2][2] = -(1 / (c + 1));
  unhingeMat4[3][3] = 0;
  unhingeMat4[2][3] = -1;
  unhingeMat4[3][2] = c / (c + 1);
  return unhingeMat4;
}

/* The resulting projection matrix (projectionMatrix) is the product of the unhinge matrix and the scale matrix:

projectionMatrix = unhingeMat4 * scaleMat4;

This matrix combines both scaling and depth mapping. It is used to project 3D objects from world space into normalized device coordinates (NDC) for rendering.
*/
glm::mat4 Camera::getProjectionMatrix() {
  glm::mat4 projectionMatrix = getUnhingeMatrix() * getScaleMatrix();
  return projectionMatrix;
}

/* The resulting model-view matrix (modelViewMat4) is:

|     rx           ux          -lx      0 |
|     ry           uy          -ly      0 |
|     rz           uz          -lz      0 |
| -(eye * r)   -(eye * u)    eye * l    1 |

Where rx, ry, rz are the components of the right vector, 
ux, uy, uz are the components of the up vector, 
lx, ly, lz are the components of the look vector, 
and eye is the camera position.

This matrix transforms world coordinates to view coordinates (camera space).
*/
glm::mat4 Camera::getModelViewMatrix() {
  glm::vec3 eyePoint = getEyePoint();
  glm::vec3 lookVector = getLookVector();
  glm::vec3 upVector = getUpVector();

  glm::vec3 target = eyePoint + lookVector;

  glm::mat4 viewMatrix = glm::lookAt(eyePoint, target, upVector);
  glm::mat4 modelViewMat4 = viewMatrix;

  return modelViewMat4;
}


/* The resulting inverse model-view matrix is the inverse of Model-View Matrix (inverse(modelViewMat4)):

   | rx  ry  rz  eye_x |
   | ux  uy  uz  eye_y |
   | -lx -ly -lz eye_z |
   |  0   0   0     1  |

   This inverse matrix transforms coordinates from camera (view) space back to world space.
*/

glm::mat4 Camera::getInverseModelViewMatrix() {
  glm::vec3 eyePoint = getEyePoint();
  glm::vec3 lookVector = getLookVector();
  glm::vec3 upVector = getUpVector();

  glm::vec3 target = eyePoint + lookVector;

  glm::mat4 viewMatrix = glm::lookAt(eyePoint, target, upVector);
  glm::mat4 modelViewMat4 = viewMatrix;
  
  return glm::inverse(modelViewMat4);
}


// 视角的范围必须在 0 到 180 度之间。因为视角过小会导致相机视野非常狭窄，而视角过大可能会产生极端的透视扭曲效果
void Camera::setViewAngle(float _viewAngle) {
  if (_viewAngle <= 0.0f || _viewAngle >= 180.0f) {
    std::cerr << "Error: View angle must be between 0 and 180 degrees. "
              << "Setting to default 60 degrees." << std::endl;
    viewAngle = glm::radians(60.0f);
  } else {
    viewAngle = glm::radians(_viewAngle);
  }
}

// 相机最近可以渲染的距离，如果距离小于等于 0，默认设置为 0.01
void Camera::setNearPlane(float _nearPlane) {
  if (_nearPlane <= 0.0f) {
    std::cerr << "Error: Near plane must be greater than 0. "
              << "Setting to default 0.01f." << std::endl;
    nearPlane = 0.01f;
  } else {
    nearPlane = _nearPlane;
  }
}

// 相机最远可以渲染的距离，它必须大于 nearPlane。如果不满足条件，默认设置为 20.0
void Camera::setFarPlane(float _farPlane) {
  if (_farPlane <= nearPlane) {
    std::cerr << "Error: Far plane must be greater than near plane. "
              << "Setting to default 20.0f." << std::endl;
    farPlane = 20.0f;
  } else {
    farPlane = _farPlane;
  }
}

// 如果传入的宽度或高度小于等于 0，会输出错误信息，并将屏幕尺寸设置为默认值 800x600。这影响了相机的宽高比（aspect ratio），在投影时，宽高比决定了图像是否会出现拉伸或压缩
void Camera::setScreenSize(int _screenWidth, int _screenHeight) {
  if (_screenWidth <= 0 || _screenHeight <= 0) {
    std::cerr << "Error: Screen dimensions must be positive. "
              << "Setting to default 800x600." << std::endl;
    screenWidth = 800;
    screenHeight = 600;
  } else {
    screenWidth = _screenWidth;
    screenHeight = _screenHeight;
  }
}
/* 
The resulting matrix for rotation around the Y-axis (upVector = (0, 1, 0)) is:
| cos(θ)  0  sin(θ)  0 |
|   0     1     0    0 |
| -sin(θ) 0  cos(θ)  0 |
|   0     0     0    1 |
*/
void Camera::rotateV(float degrees) {
  float radians = glm::radians(degrees);

  if (glm::length(lookVector) == 0.0f || glm::length(upVector) == 0.0f) {
    std::cerr << "Error: lookVector or upVector is a zero vector!" << std::endl;
    return;
  }

  glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), radians, upVector);

  glm::vec3 newLook = glm::vec3(rotationMatrix * glm::vec4(lookVector, 0.0f));
  glm::vec3 newRight = glm::normalize(glm::cross(newLook, upVector));

  lookVector = glm::normalize(newLook);
}

/* 
The resulting matrix for rotation around the U axis (right vector) is:
| 1     0        0       0 |
| 0   cos(θ)  -sin(θ)   0 |
| 0   sin(θ)   cos(θ)   0 |
| 0     0        0       1 |
*/

void Camera::rotateU(float degrees) {
  float radians = glm::radians(degrees);

  if (glm::length(lookVector) == 0.0f || glm::length(upVector) == 0.0f) {
    std::cerr << "Error: lookVector or upVector is a zero vector!" << std::endl;
    return;
  }

  glm::vec3 right = glm::cross(lookVector, upVector);
  if (glm::length(right) == 0.0f) {
    std::cerr << "Error: lookVector and upVector are parallel!" << std::endl;
    return;
  }
  right = glm::normalize(right);

  glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), radians, right);

  glm::vec3 newLook = glm::vec3(rotationMatrix * glm::vec4(lookVector, 0.0f));
  glm::vec3 newUp = glm::vec3(rotationMatrix * glm::vec4(upVector, 0.0f));

  lookVector = glm::normalize(newLook);
  upVector = glm::normalize(newUp);
}

/* 
The resulting matrix for rotation around the W axis (look vector) is:
| cos(θ)  -sin(θ)   0   0 |
| sin(θ)   cos(θ)   0   0 |
|   0        0      1   0 |
|   0        0      0   1 |
*/

void Camera::rotateW(float degrees) {

  float radians = glm::radians(degrees);

  if (glm::length(lookVector) == 0.0f || glm::length(upVector) == 0.0f) {
    std::cerr << "Error: lookVector or upVector is a zero vector!" << std::endl;
    return;
  }

  glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), radians, lookVector);
  glm::vec3 newUp = glm::vec3(rotationMatrix * glm::vec4(upVector, 0.0f));
  glm::vec3 newRight = glm::normalize(glm::cross(lookVector, newUp));

  upVector = glm::normalize(newUp);
}

void Camera::translate(glm::vec3 v) { // v 表示的是相机空间中的方向
    glm::vec3 globalTransMat = getInverseModelViewMatrix() * glm::vec4(v, 0.0f); // 世界坐标系中，平移向量 v 所对应的方向和大小
    lookVector = lookVector + globalTransMat ;
}

void Camera::rotate(glm::vec3 point, glm::vec3 axis, float degrees) {
  float radians = glm::radians(degrees);
  glm::mat4 rotationMatrix = glm::translate(glm::mat4(1.0f), point) * // 3.将场景中的对象 平移回去，把它们从原点移动回 point 位置
                             glm::rotate(glm::mat4(1.0f), radians, axis) * // 2.绕 axis 轴进行 旋转，因为 point 已经被移动到原点，所以旋转会围绕原点进行
                             glm::translate(glm::mat4(1.0f), -point); // 1.将场景中的对象 平移 使 point 位置移动到原点

  lookVector =
      glm::normalize(glm::vec3(rotationMatrix * glm::vec4(lookVector, 0.0f)));
  upVector =
      glm::normalize(glm::vec3(rotationMatrix * glm::vec4(upVector, 0.0f)));
}

glm::vec3 Camera::getEyePoint() { return position; }

glm::vec3 Camera::getLookVector() { return lookVector; }

glm::vec3 Camera::getUpVector() { return upVector; }

float Camera::getViewAngle() { return glm::degrees(viewAngle); }

float Camera::getNearPlane() { return nearPlane; }

float Camera::getFarPlane() { return farPlane; }

int Camera::getScreenWidth() { return screenWidth; }

int Camera::getScreenHeight() { return screenHeight; }

float Camera::getScreenWidthRatio() { return screenWidthRatio; }

// Logs
bool isZeroVector(const glm::vec3 &vec) {
  return vec.x == 0.0f && vec.y == 0.0f && vec.z == 0.0f;
}

void Camera::printCamVec(const std::string &info) {
  std::printf("%s\n", info.c_str());
  if (isZeroVector(position)) {
    std::printf("Warning: position is uninitialized or zero vector!\n");
  } else {
    std::printf("position: (%f, %f, %f)\n", position.x, position.y, position.z);
  }

  // print lookVector
  if (isZeroVector(lookVector)) {
    std::printf("Warning: lookVector is uninitialized or zero vector!\n");
  } else {
    std::printf("lookVector: (%f, %f, %f)\n", lookVector.x, lookVector.y,
                lookVector.z);
  }

  // print upVector
  if (isZeroVector(upVector)) {
    std::printf("Warning: upVector is uninitialized or zero vector!\n");
  } else {
    std::printf("upVector: (%f, %f, %f)\n", upVector.x, upVector.y, upVector.z);
  }
}

void Camera::printMat4(const glm::mat4 &mat) {
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      std::cout << mat[row][col] << " ";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
}
