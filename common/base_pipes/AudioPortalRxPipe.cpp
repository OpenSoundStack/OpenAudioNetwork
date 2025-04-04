#include "AudioPortalRxPipe.h"

AudioPortalRxPipe::AudioPortalRxPipe(uint8_t channel, uint16_t src_uid, const std::shared_ptr<LowLatSocket> &iface) {
    m_iface = iface;

    m_channel = channel;
    m_src_uid = src_uid;

    m_rx_packet = {};
}

void AudioPortalRxPipe::acquire_sample(float sample) {
    if (m_iface->receive_data(&m_rx_packet) <= 0) {
        return;
    }

    bool is_for_me = m_rx_packet.llhdr.sender_uid == m_src_uid && m_rx_packet.payload.packet_data.channel == m_channel;
    if (is_for_me && m_rx_packet.payload.type == PacketType::AUDIO) {
        for (int i = 0; i < AUDIO_DATA_SAMPLES_PER_PACKETS; i++) {
            forward_sample(m_rx_packet.payload.packet_data.samples[i]);
        }
    }
}

