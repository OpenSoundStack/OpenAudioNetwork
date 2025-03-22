#ifndef AUDIOPORTALPIPE_H
#define AUDIOPORTALPIPE_H

#include "common/AudioPipe.h"
#include "netutils/udp.h"
#include "common/packet_structs.h"

class AudioPortalPipe : public AudioPipe {
public:
    AudioPortalPipe(uint8_t channel, uint32_t ip, uint16_t port, const std::shared_ptr<UDPSocket>& iface);

    void acquire_sample(float sample) override;
private:
    void push_packet();

    AudioPacket m_portal_packet{};
    int m_sample_idx;

    uint32_t m_dest_ip;
    uint16_t m_dest_port;

    std::shared_ptr<UDPSocket> m_dest_sock;
    uint8_t m_channel_no;
};



#endif //AUDIOPORTALPIPE_H
