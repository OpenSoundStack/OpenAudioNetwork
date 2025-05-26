#ifndef AUDIOPIPE_H
#define AUDIOPIPE_H

#include <memory>
#include <optional>

#include "packet_structs.h"

/**
 * @class AudioPipe
 * @brief Represents a list of pipe elements. Each pipe element does one processing. The whole pipeline forms the processing chain.
 */
class AudioPipe {
public:
    AudioPipe();
    virtual ~AudioPipe() = default;

    /**
     * Push an audio packet in the pipe
     * @param pck An audio packet
     */
    void feed_packet(AudioPacket& pck);

    /**
     * Install nex pipe element in chain
     * @param pipe Next pipe
     */
    void set_next_pipe(const std::shared_ptr<AudioPipe>& pipe);

    /**
     * If it exists, returns the next pipe elem in chain
     * @return
     */
    std::optional<std::shared_ptr<AudioPipe>> next_pipe();

    bool is_pipe_enabled() const;
    void set_pipe_enabled(bool en);

    void set_channel(uint8_t channel);
    uint8_t get_channel();

    /**
     * Apply a control packet to the elem
     * @param pck
     */
    virtual void apply_control(ControlPacket& pck);

protected:
    /**
     * Sends processed audio to the next pipe element
     * @param pck Processed audio packet
     */
    void forward_sample(AudioPacket& pck);

    /**
     * Process a unique sample
     * @param sample Audio sample
     * @return The processed sample
     */
    virtual float process_sample(float sample);

private:
    std::optional<std::shared_ptr<AudioPipe>> m_next_pipe;

    // Pipe meta info
    bool m_pipe_enabled;
    uint8_t m_channel_no;
};



#endif //AUDIOPIPE_H
