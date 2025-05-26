#ifndef AUDIOINPIPE_H
#define AUDIOINPIPE_H

#include "common/AudioPipe.h"

class AudioInPipe : public AudioPipe {
public:
    AudioInPipe();

    float process_sample(float sample) override;
    void set_gain_lin(float gain);
    void set_trim_lin(float trim);

    void apply_control(ControlPacket &pck) override;
private:
    float m_in_gain;
    float m_in_trim;
};



#endif //AUDIOINPIPE_H
