#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <iomanip>

// 常量定义
const int N = 32;        // 网格分辨率 (N x N)
const float L = 64.0f;   // 网格的物理尺寸
const float GRAVITY = 9.81f;  // 重力加速度
const float WAVE_AMP = 0.0002f; // 能量缩放因子
const float WIND_SPEED = 32.0f; // 风速
const float DAMPING = 0.001f;   // 阻尼系数

// 波浪数据结构
struct WaveData {
    float dirX, dirZ;     // 波浪方向 (单位化)
    float omega;          // 波浪角速度
    float amplitude;      // 波浪振幅
    float phaseOffset;    // 初始相位偏移
};

// 生成 Phillips 光谱数据
std::vector<WaveData> generatePhillipsSpectrum() {
    std::vector<WaveData> waves;
    std::default_random_engine generator(42);  // 固定随机种子
    std::uniform_real_distribution<float> phaseDist(0.0f, 2.0f * M_PI);

    const float L_wind = (WIND_SPEED * WIND_SPEED) / GRAVITY;
    const float l2 = L_wind * L_wind * DAMPING * DAMPING;

    for (int m_prime = 0; m_prime < N / 2; ++m_prime) {  // 只计算半个波矢量空间
        for (int n_prime = 0; n_prime < N; ++n_prime) {
            // 计算波矢量 k
            float kx = M_PI * (2 * n_prime - N) / L;
            float kz = M_PI * (2 * m_prime - N) / L;
            float k_length = std::sqrt(kx * kx + kz * kz);

            // 过滤 k = 0
            if (k_length < 1e-6) continue;

            float k_unitX = kx / k_length;
            float k_unitZ = kz / k_length;
            float k_dot_wind = k_unitX * 1.0f + k_unitZ * 0.0f;  // 假设风向为 (1, 0)

            // 过滤反向波浪
            if (k_dot_wind < 0) continue;

            // Phillips 光谱公式
            float k_length2 = k_length * k_length;
            float k_length4 = k_length2 * k_length2;

            float phillips = WAVE_AMP * std::exp(-1.0f / (k_length2 * L_wind * L_wind)) / k_length4;
            phillips *= std::pow(k_dot_wind, 6) * std::exp(-k_length2 * l2);

            if (phillips < 1e-8) continue;

            // 保存数据
            WaveData wave;
            wave.dirX = k_unitX;
            wave.dirZ = k_unitZ;
            wave.omega = std::sqrt(GRAVITY * k_length);
            wave.amplitude = std::sqrt(phillips);
            wave.phaseOffset = phaseDist(generator);

            waves.push_back(wave);
        }
    }
    return waves;
}

// 格式化输出 GLSL 数组
void printGLSLFormatted(const std::vector<WaveData>& waves) {
    std::cout << "const int freqCount = " << waves.size() << ";\n\n";

    // freqDir
    std::cout << "const vec2 freqDir[" << waves.size() << "] = vec2[](\n";
    for (size_t i = 0; i < waves.size(); ++i) {
        std::cout << "    vec2(" << std::fixed << std::setprecision(6)
                  << waves[i].dirX << ", " << waves[i].dirZ << ")";
        if (i != waves.size() - 1) std::cout << ",";
        if ((i + 1) % 4 == 0 || i == waves.size() - 1) std::cout << "\n";
    }
    std::cout << ");\n\n";

    // freqOmega
    std::cout << "const float freqOmega[" << waves.size() << "] = float[](\n";
    for (size_t i = 0; i < waves.size(); ++i) {
        std::cout << "    " << std::fixed << std::setprecision(6) << waves[i].omega;
        if (i != waves.size() - 1) std::cout << ",";
        if ((i + 1) % 4 == 0 || i == waves.size() - 1) std::cout << "\n";
    }
    std::cout << ");\n\n";

    // freqAmp
    std::cout << "const float freqAmp[" << waves.size() << "] = float[](\n";
    for (size_t i = 0; i < waves.size(); ++i) {
        std::cout << "    " << std::fixed << std::setprecision(6) << waves[i].amplitude;
        if (i != waves.size() - 1) std::cout << ",";
        if ((i + 1) % 4 == 0 || i == waves.size() - 1) std::cout << "\n";
    }
    std::cout << ");\n\n";

    // freqPhaseOffset
    std::cout << "const float freqPhaseOffset[" << waves.size() << "] = float[](\n";
    for (size_t i = 0; i < waves.size(); ++i) {
        std::cout << "    " << std::fixed << std::setprecision(6) << waves[i].phaseOffset;
        if (i != waves.size() - 1) std::cout << ",";
        if ((i + 1) % 4 == 0 || i == waves.size() - 1) std::cout << "\n";
    }
    std::cout << ");\n";
}

