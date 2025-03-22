#ifndef AUDIOINPIPE_H
#define AUDIOINPIPE_H

#include "common/AudioPipe.h"

class AudioInPipe : public AudioPipe {
public:
    AudioInPipe();

    void process_sample(float sample) override;
    void set_gain_lin(float gain);

private:
    float m_in_gain;
};



#endif //AUDIOINPIPE_H
