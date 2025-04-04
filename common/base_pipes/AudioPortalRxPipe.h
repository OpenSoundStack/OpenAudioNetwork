#ifndef AUDIOPORTALRXPIPE_H
#define AUDIOPORTALRXPIPE_H

#include "common/AudioPipe.h"
#include "common/packet_structs.h"
#include "netutils/LowLatSocket.h"

class AudioPortalRxPipe : public AudioPipe {
public:
    AudioPortalRxPipe(uint8_t channel, uint16_t src_uid, const std::shared_ptr<LowLatSocket>& iface);
    virtual ~AudioPortalRxPipe() = default;

    void acquire_sample(float sample) override;

private:
    std::shared_ptr<LowLatSocket> m_iface;
    LowLatPacket<AudioPacket> m_rx_packet;

    uint8_t m_channel;
    uint16_t m_src_uid;
};



#endif //AUDIOPORTALRXPIPE_H
