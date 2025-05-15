#include "LevelMeasurePipe.h"

#include <cmath>

LevelMeasurePipe::LevelMeasurePipe() {
    m_sum = 0.0f;
}


float LevelMeasurePipe::process_sample(float sample) {
    static int value_counter = 0;
    constexpr float max_level = (float)(1 << 24);

    m_sum += abs(sample);

    value_counter++;
    if (value_counter > 960) {
        float mean = m_sum / value_counter;
        float mean_db = 20 * std::log10(mean / max_level);

        std::cout << "Mean level : " << mean_db << " db" << std::endl;

        value_counter = 0;
        m_sum = 0.0f;
    }

    return sample;
}

