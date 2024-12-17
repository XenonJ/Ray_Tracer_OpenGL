#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <iomanip>

const int N = 32;
const float L = 64.0f;
const float GRAVITY = 9.81f;
const float WAVE_AMP = 0.0002f;
const float WIND_SPEED = 32.0f;
const float DAMPING = 0.001f;

struct WaveData {
    float dirX, dirZ;
    float omega;
    float amplitude;
    float phaseOffset;
};

// 1. Wave Vector k:
//    k = (kx, kz), computed as:
//        kx = π * (2n' - N) / L, kz = π * (2m' - N) / L
//
// 2. Phillips Spectrum:
//    P(k) = A * exp(-1 / (k^2 * L_wind^2)) / k^4 * (k · w)^6 * exp(-k^2 * l^2)
//
// 3. Filtering:
//    - k = 0: Skip zero vectors.
//    - (k · w) < 0: Ignore reverse waves.
//    - Small P(k): Skip negligible energy.

std::vector<WaveData> generatePhillipsSpectrum() {
    std::vector<WaveData> waves;
    std::default_random_engine generator(42);
    std::uniform_real_distribution<float> phaseDist(0.0f, 2.0f * M_PI);

    const float L_wind = (WIND_SPEED * WIND_SPEED) / GRAVITY;
    const float l2 = L_wind * L_wind * DAMPING * DAMPING;

    for (int m_prime = 0; m_prime < N / 2; ++m_prime) {
        for (int n_prime = 0; n_prime < N; ++n_prime) {

            float kx = M_PI * (2 * n_prime - N) / L;
            float kz = M_PI * (2 * m_prime - N) / L;
            float k_length = std::sqrt(kx * kx + kz * kz);

            if (k_length < 1e-6) continue;

            float k_unitX = kx / k_length;
            float k_unitZ = kz / k_length;
            float k_dot_wind = k_unitX * 1.0f + k_unitZ * 0.0f;  // wind direction (1, 0)

            if (k_dot_wind < 0) continue;

            // Phillips Spectrum
            float k_length2 = k_length * k_length;
            float k_length4 = k_length2 * k_length2;

            float phillips = WAVE_AMP * std::exp(-1.0f / (k_length2 * L_wind * L_wind)) / k_length4;
            phillips *= std::pow(k_dot_wind, 6) * std::exp(-k_length2 * l2);

            if (phillips < 1e-8) continue;

            // Save data
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
