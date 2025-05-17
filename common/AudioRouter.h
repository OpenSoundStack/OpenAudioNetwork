#ifndef AUDIOROUTER_H
#define AUDIOROUTER_H

#include "netutils/LowLatSocket.h"
#include "packet_structs.h"

#include <functional>

class AudioRouter {
public:
    AudioRouter(uint16_t self_uid);
    ~AudioRouter() = default;

    bool init_router(const std::string& eth_interface, const std::shared_ptr<NetworkMapper>& nmapper);

    void poll_audio_data();
    void poll_control_packets();

    void send_audio_packet(const AudioPacket &packet, uint16_t dest_uid);
    void send_control_packet_response(const ControlResponsePacket& packet, uint16_t dest_uid);

    void set_routing_callback(const std::function<void(AudioPacket&, LowLatHeader&)> &callback);
    void set_control_callback(const std::function<void(ControlPacket&, LowLatHeader&)>& callback);
    void set_pipe_create_callback(const std::function<void(ControlPipeCreatePacket&, LowLatHeader&)>& callback);
private:
    std::unique_ptr<LowLatSocket> m_audio_iface;
    std::unique_ptr<LowLatSocket> m_control_iface;
    uint16_t m_self_uid;

    std::function<void(AudioPacket&, LowLatHeader&)> m_routing_callback;
    std::function<void(ControlPacket&, LowLatHeader&)> m_channel_control_callback;
    std::function<void(ControlPipeCreatePacket&, LowLatHeader&)> m_pipe_create_callback;
};



#endif //AUDIOROUTER_H
