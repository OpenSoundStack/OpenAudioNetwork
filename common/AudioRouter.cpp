#include "AudioRouter.h"

AudioRouter::AudioRouter(const std::string &eth_interface, uint16_t self_uid, const std::shared_ptr<NetworkMapper>& nmapper) {
    m_audio_iface = std::make_unique<LowLatSocket>(self_uid, nmapper);
    m_audio_iface->init_socket(eth_interface, ETH_PROTO_OANAUDIO);

    m_self_uid = self_uid;
    m_routing_callback = [](const AudioPacket&) {};
}

void AudioRouter::poll_audio_data() {
    LowLatPacket<AudioPacket> rx_packet;
    if (m_audio_iface->receive_data<LowLatPacket<AudioPacket>>(&rx_packet) <= 0) {
        return;
    }

    if (rx_packet.llhdr.dest_uid == m_self_uid && rx_packet.payload.type == PacketType::AUDIO) {
        m_routing_callback(rx_packet.payload);
    }
}

void AudioRouter::send_audio_packet(const AudioPacket &packet, uint16_t dest_uid) {
    m_audio_iface->send_data(packet, dest_uid);
}

void AudioRouter::set_routing_callback(std::function<void(const AudioPacket &)> callback) {
    m_routing_callback = callback;
}
