#ifndef LEVELMEASUREPIPE_H
#define LEVELMEASUREPIPE_H

#include <iostream>

#include "common/AudioPipe.h"

class LevelMeasurePipe : public AudioPipe {
public:
    LevelMeasurePipe();
    virtual ~LevelMeasurePipe() = default;

    float process_sample(float sample) override;

private:
    float m_sum;
};



#endif //LEVELMEASUREPIPE_H
