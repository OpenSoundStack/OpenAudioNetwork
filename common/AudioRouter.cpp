// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2025 - Mathis DELGADO
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, version 3 of the License.
//
// This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

#include "AudioRouter.h"

AudioRouter::AudioRouter(uint16_t self_uid) {
    m_self_uid = self_uid;
    m_routing_callback = [](AudioPacket&, LowLatHeader&) {};
    m_channel_control_callback = [](ControlPacket&, LowLatHeader&) {};
    m_pipe_create_callback = [](ControlPipeCreatePacket&, LowLatHeader&) {};
}

bool AudioRouter::init_router(const std::string &eth_interface, const std::shared_ptr<NetworkMapper>& nmapper) {
    m_audio_iface = std::make_unique<LowLatSocket>(m_self_uid, nmapper);
    if (!m_audio_iface->init_socket(eth_interface, ETH_PROTO_OANAUDIO)) {
        return false;
    }

    m_control_iface = std::make_unique<LowLatSocket>(m_self_uid, nmapper);
    if (!m_control_iface->init_socket(eth_interface, ETH_PROTO_OANCONTROL)) {
        return false;
    }

    return true;
}


void AudioRouter::poll_audio_data() {
    LowLatPacket<AudioPacket> rx_packet{};
    if (m_audio_iface->receive_data<LowLatPacket<AudioPacket>>(&rx_packet) <= 0) {
        return;
    }

    if (rx_packet.llhdr.dest_uid == m_self_uid && rx_packet.payload.header.type == PacketType::AUDIO) {
        AudioPacket pck = rx_packet.payload;
        m_routing_callback(pck, rx_packet.llhdr);
    }
}

void AudioRouter::poll_control_packets(bool async) {
    char raw_packet_buffer[256] = {0};
    LowLatPacket<CommonHeader> header{};

    int recv_bytes = m_control_iface->receive_data_raw(raw_packet_buffer, 128, async);

    if (recv_bytes <= 0) {
        return;
    } else {
        // Getting the header for analysis
        memcpy(&header, raw_packet_buffer, sizeof(LowLatPacket<CommonHeader>));

        if (header.llhdr.dest_uid != m_self_uid) {
            return;
        }

        // Packet switching
        if (header.payload.type == PacketType::CONTROL_CREATE) {
            ControlPipeCreatePacket packet_content{};

            // Extracting packet
            packet_content.header = header.payload;
            memcpy(&packet_content.packet_data, raw_packet_buffer + sizeof(LowLatPacket<CommonHeader>), sizeof(ControlPipeCreate));

            m_pipe_create_callback(packet_content, header.llhdr);
        } else if (header.payload.type == PacketType::CONTROL) {
            ControlPacket packet_content{};

            // Extracting packet
            packet_content.header = header.payload;
            memcpy(&packet_content.packet_data, raw_packet_buffer + sizeof(LowLatPacket<CommonHeader>), sizeof(ControlData));

            m_channel_control_callback(packet_content, header.llhdr);
        } else if (header.payload.type == PacketType::CONTROL_RESPONSE) {
            ControlResponsePacket packet_content{};

            packet_content.header = header.payload;
            memcpy(&packet_content.packet_data, raw_packet_buffer + sizeof(LowLatPacket<CommonHeader>), sizeof(ControlResponse));

            m_control_response_callback(packet_content, header.llhdr);
        } else if (header.payload.type == PacketType::CONTROL_QUERY) {
            ControlQueryPacket packet_content{};

            // Extracting packet
            packet_content.header = header.payload;
            memcpy(&packet_content.packet_data, raw_packet_buffer + sizeof(LowLatPacket<CommonHeader>), sizeof(ControlQuery));

            m_control_query_callback(packet_content, header.llhdr);
        }
    }
}

void AudioRouter::send_audio_packet(const AudioPacket &packet, uint16_t dest_uid) {
    m_audio_iface->send_data(packet, dest_uid);
}

void AudioRouter::set_routing_callback(const std::function<void(AudioPacket&, LowLatHeader&)> &callback) {
    m_routing_callback = callback;
}

void AudioRouter::set_control_callback(const std::function<void(ControlPacket&, LowLatHeader&)>& callback) {
    m_channel_control_callback = callback;
}

void AudioRouter::set_pipe_create_callback(const std::function<void(ControlPipeCreatePacket&, LowLatHeader&)>& callback) {
    m_pipe_create_callback = callback;
}

void AudioRouter::set_control_response_callback(const std::function<void(ControlResponsePacket &, LowLatHeader &)> &callback) {
    m_control_response_callback = callback;
}

void AudioRouter::set_control_query_callback(const std::function<void(ControlQueryPacket &, LowLatHeader &)> &callback) {
    m_control_query_callback = callback;
}

