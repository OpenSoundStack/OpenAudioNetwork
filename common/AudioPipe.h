#ifndef AUDIOPIPE_H
#define AUDIOPIPE_H

#include <memory>
#include <optional>

#include "packet_structs.h"

class AudioPipe {
public:
    AudioPipe();
    virtual ~AudioPipe() = default;

    void feed_packet(AudioPacket& pck);
    void set_next_pipe(std::unique_ptr<AudioPipe>&& pipe);

    bool is_pipe_enabled() const;
    void set_pipe_enabled(bool en);

    void set_channel(uint8_t channel);
    uint8_t get_channel();
protected:
    void forward_sample(AudioPacket& pck);
    virtual float process_sample(float sample);

private:
    std::optional<std::unique_ptr<AudioPipe>> m_next_pipe;

    // Pipe meta info
    bool m_pipe_enabled;
    uint8_t m_channel_no;
};



#endif //AUDIOPIPE_H
