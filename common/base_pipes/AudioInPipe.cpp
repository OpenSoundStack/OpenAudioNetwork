#include "AudioInPipe.h"

AudioInPipe::AudioInPipe() {
    m_in_gain = 1; // 0dB gain by default
}

float AudioInPipe::process_sample(float sample) {
    return (sample * m_in_gain);
}

void AudioInPipe::set_gain_lin(float gain) {
    m_in_gain = gain;
}
