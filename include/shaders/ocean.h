#ifndef OCEAN_H
#define OCEAN_H

#include <vector>

struct WaveData {
    float dirX, dirZ;
    float omega;
    float amplitude;
    float phaseOffset;
};

std::vector<WaveData> generatePhillipsSpectrum();

#endif // OCEAN_H
