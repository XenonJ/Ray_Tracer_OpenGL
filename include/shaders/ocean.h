#ifndef OCEAN_H
#define OCEAN_H

#include <vector>

// 波浪数据结构
struct WaveData {
    float dirX, dirZ;     // 波浪方向 (单位化)
    float omega;          // 波浪角速度
    float amplitude;      // 波浪振幅
    float phaseOffset;    // 初始相位偏移
};

// 函数声明：生成 Phillips 光谱数据
std::vector<WaveData> generatePhillipsSpectrum();

// 函数声明：将数据格式化输出为 GLSL 数组
void printGLSLFormatted(const std::vector<WaveData>& waves);

#endif // OCEAN_H
