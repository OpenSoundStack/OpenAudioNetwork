#include "AudioPortalPipe.h"

AudioPortalPipe::AudioPortalPipe(uint8_t channel, uint16_t dest_uid, const std::shared_ptr<LowLatSocket>& iface) {
    m_channel_no = channel;
    m_dest_sock = iface;

    m_dest_uid = dest_uid;

    m_portal_packet = {};
    m_portal_packet.type = PacketType::AUDIO;
    m_portal_packet.packet_data.channel = channel;

    m_sample_idx = 0;
}

void AudioPortalPipe::acquire_sample(float sample) {
    m_portal_packet.packet_data.samples[m_sample_idx] = sample;

    m_sample_idx++;
    if (m_sample_idx == 64) {
        push_packet();
        m_sample_idx = 0;
    }
}

void AudioPortalPipe::push_packet() {
    if (m_dest_sock->send_data(m_portal_packet, m_dest_uid) < 0) {
        std::cerr << "FAIL" << std::endl;
    }
}
