#ifndef AUDIOROUTER_H
#define AUDIOROUTER_H

#include "netutils/LowLatSocket.h"
#include "packet_structs.h"

#include <functional>

class AudioRouter {
public:
    AudioRouter(const std::string& eth_interface, uint16_t self_uid, const std::shared_ptr<NetworkMapper>& nmapper);
    ~AudioRouter() = default;

    void poll_audio_data();
    void send_audio_packet(const AudioPacket &packet, uint16_t dest_uid);

    void set_routing_callback(std::function<void(AudioPacket&)> callback);

private:
    std::unique_ptr<LowLatSocket> m_audio_iface;
    uint16_t m_self_uid;

    std::function<void(AudioPacket&)> m_routing_callback;
};



#endif //AUDIOROUTER_H
