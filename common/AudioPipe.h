#ifndef AUDIOPIPE_H
#define AUDIOPIPE_H

#include <memory>
#include <optional>

class AudioPipe {
public:
    AudioPipe();
    virtual ~AudioPipe() = default;

    virtual void acquire_sample(float sample);
    virtual void process_sample(float sample);

    void set_next_pipe(std::unique_ptr<AudioPipe>&& pipe);

protected:
    void forward_sample(float sample);

private:
    std::optional<std::unique_ptr<AudioPipe>> m_next_pipe;
};



#endif //AUDIOPIPE_H
