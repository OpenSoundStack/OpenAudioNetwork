#include "AudioPipe.h"

AudioPipe::AudioPipe() {
    m_next_pipe = {};
}

void AudioPipe::acquire_sample(float sample) {
    process_sample(sample);
}

void AudioPipe::process_sample(float sample) {
    forward_sample(sample);
}

void AudioPipe::forward_sample(float sample) {
    if (m_next_pipe.has_value()) {
        m_next_pipe.value()->acquire_sample(sample);
    }
}

void AudioPipe::set_next_pipe(std::unique_ptr<AudioPipe>&& pipe) {
    m_next_pipe = std::move(pipe);
}

