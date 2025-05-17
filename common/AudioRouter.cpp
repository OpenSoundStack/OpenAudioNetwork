#include "AudioRouter.h"

AudioRouter::AudioRouter(const std::string &eth_interface, uint16_t self_uid, const std::shared_ptr<NetworkMapper>& nmapper) {
    m_audio_iface = std::make_unique<LowLatSocket>(self_uid, nmapper);
    m_audio_iface->init_socket(eth_interface, ETH_PROTO_OANAUDIO);

    m_control_iface = std::make_unique<LowLatSocket>(self_uid, nmapper);
    m_control_iface->init_socket(eth_interface, ETH_PROTO_OANCONTROL);

    m_self_uid = self_uid;
    m_routing_callback = [](AudioPacket&) {};
}

void AudioRouter::poll_audio_data() {
    LowLatPacket<AudioPacket> rx_packet{};
    if (m_audio_iface->receive_data<LowLatPacket<AudioPacket>>(&rx_packet) <= 0) {
        return;
    }

    if (rx_packet.llhdr.dest_uid == m_self_uid && rx_packet.payload.header.type == PacketType::AUDIO) {
        AudioPacket pck = rx_packet.payload;
        m_routing_callback(pck);
    }
}

void AudioRouter::poll_control_packets() {
    char raw_packet_buffer[256] = {0};
    LowLatPacket<CommonHeader> header{};

    int recv_bytes = m_control_iface->receive_data_raw(raw_packet_buffer, 128);

    // Getting the header for analysis
    memcpy(&header, raw_packet_buffer, sizeof(LowLatPacket<CommonHeader>));

    if (recv_bytes <= 0) {
        return;
    } else {
        if (header.payload.type == PacketType::CONTROL_CREATE) {
            ControlPipeCreatePacket packet_content{};

            // Extracting packet
            packet_content.header = header.payload;
            memcpy(&packet_content.packet_data, raw_packet_buffer + sizeof(LowLatPacket<CommonHeader>), sizeof(ControlPipeCreate));

            m_pipe_create_callback(packet_content);
        } else if (header.payload.type == PacketType::CONTROL) {
            ControlPacket packet_content{};

            // Extracting packet
            packet_content.header = header.payload;
            memcpy(&packet_content, raw_packet_buffer + sizeof(LowLatPacket<CommonHeader>), sizeof(ControlData));

            m_channel_control_callback(packet_content);
        }
    }
}

void AudioRouter::send_audio_packet(const AudioPacket &packet, uint16_t dest_uid) {
    m_audio_iface->send_data(packet, dest_uid);
}

void AudioRouter::set_routing_callback(const std::function<void(AudioPacket &)> &callback) {
    m_routing_callback = callback;
}

void AudioRouter::set_control_callback(const std::function<void(ControlPacket&)>& callback) {
    m_channel_control_callback = callback;
}

void AudioRouter::set_pipe_create_callback(const std::function<void(ControlPipeCreatePacket &)>& callback) {
    m_pipe_create_callback = callback;
}

