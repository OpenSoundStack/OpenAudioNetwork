// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#include "AudioRouter.h"

#include <algorithm>

#include "NetworkMapper.h"

AudioRouter::AudioRouter(uint16_t self_uid) {
    m_self_uid = self_uid;
    m_routing_callback = [](AudioPacket&, LowLatHeader&, bool) {};
    m_channel_control_callback = [](ControlPacket&, LowLatHeader&) {};
    m_pipe_create_callback = [](ControlPipeCreatePacket&, LowLatHeader&) {};

    m_transaction_tracker = 0;
}

bool AudioRouter::init_router(const std::string &eth_interface, const std::shared_ptr<NetworkMapper>& nmapper) {
    m_nmapper = nmapper;

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

void AudioRouter::poll_local_audio_buffer() {
    AudioPacket local_packet;

    while (m_local_audio_fifo.try_dequeue(local_packet)) {
        LowLatHeader llhdr{};
        llhdr.sender_uid = m_self_uid;
        llhdr.dest_uid = m_self_uid;

        m_routing_callback(local_packet, llhdr, true);
    }
}


void AudioRouter::poll_audio_data(bool async) {
    LowLatPacket<AudioPacket> rx_packet{};
    if (m_audio_iface->receive_data<LowLatPacket<AudioPacket>>(&rx_packet, async) <= 0) {
        return;
    }

    if (rx_packet.llhdr.dest_uid == m_self_uid && rx_packet.payload.header.type == PacketType::AUDIO) {
        AudioPacket pck = rx_packet.payload;
        m_routing_callback(pck, rx_packet.llhdr, false);
    }
}

void AudioRouter::poll_control_packets(bool async) {
    char raw_packet_buffer[256] = {0};
    auto* header = reinterpret_cast<LowLatPacket<CommonHeader>*>(raw_packet_buffer);

    int recv_bytes = m_control_iface->receive_data_raw(raw_packet_buffer, 128, async);

    if (recv_bytes <= 0) {
        return;
    } else {
        if (header->llhdr.dest_uid != m_self_uid) {
            return;
        }

        // If we don't know the sender yet, keep it in memory to avoid
        // responses to incoming packet to be dropped
        if (!m_nmapper->get_mac_by_uid(header->llhdr.sender_uid).has_value()) {
            PeerInfos pinfos{};
            pinfos.peer_data.self_uid = header->llhdr.sender_uid;
            memcpy(&pinfos.peer_data.self_address, header->eth_header.h_source, 6);

            m_nmapper->add_temp_peer(header->llhdr.sender_uid, pinfos);
        }

        // Packet switching
        if (header->payload.type == PacketType::CONTROL_CREATE) {
            ControlPipeCreatePacket packet_content{};

            // Extracting packet
            packet_content.header = header->payload;
            memcpy(&packet_content.packet_data, raw_packet_buffer + sizeof(LowLatPacket<CommonHeader>), sizeof(ControlPipeCreate));

            m_pipe_create_callback(packet_content, header->llhdr);
        } else if (header->payload.type == PacketType::CONTROL) {
            ControlPacket packet_content{};

            // Extracting packet
            packet_content.header = header->payload;
            memcpy(&packet_content.packet_data, raw_packet_buffer + sizeof(LowLatPacket<CommonHeader>), sizeof(ControlData));

            m_channel_control_callback(packet_content, header->llhdr);
        } else if (header->payload.type == PacketType::CONTROL_RESPONSE) {
            ControlResponsePacket packet_content{};

            packet_content.header = header->payload;
            memcpy(&packet_content.packet_data, raw_packet_buffer + sizeof(LowLatPacket<CommonHeader>), sizeof(ControlResponse));

            m_control_response_callback(packet_content, header->llhdr);
        } else if (header->payload.type == PacketType::CONTROL_QUERY) {
            ControlQueryPacket packet_content{};

            // Extracting packet
            packet_content.header = header->payload;
            memcpy(&packet_content.packet_data, raw_packet_buffer + sizeof(LowLatPacket<CommonHeader>), sizeof(ControlQuery));

            m_control_query_callback(packet_content, header->llhdr);
        }
    }
}

void AudioRouter::send_audio_packet(const AudioPacket &packet, uint16_t dest_uid) {
    if (dest_uid != m_self_uid) {
        m_audio_iface->send_data(packet, dest_uid);
    } else {
        m_local_audio_fifo.enqueue(packet);
    }
}

void AudioRouter::set_routing_callback(const std::function<void(AudioPacket&, LowLatHeader&, bool)> &callback) {
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

void AudioRouter::create_outbound_route(uint8_t source_channel, uint16_t destination_uid) {
    std::cout << "Created outbound route : Channel " << (int)source_channel << " routed to device " << (int)destination_uid << std::endl;

    m_outbound_routes.emplace_back(
        source_channel,
        destination_uid
    );
}

void AudioRouter::create_inbound_route(uint8_t source_channel, uint16_t source_uid, uint8_t local_channel) {
    std::cout << "Created inbound route : Stream (channel " <<
        (int)source_channel << ", device " << (int)source_uid << ")" <<
        " locally routed to self channel " << (int)local_channel << std::endl;

    m_inbound_routes.emplace_back(
        source_channel,
        source_uid,
        local_channel
    );
}

std::optional<OutboundAudioRoute> AudioRouter::get_outbound_route(uint8_t source_channel) {
    for (auto& route : m_outbound_routes) {
        if (route.source_channel == source_channel) {
            return route;
        }
    }

    return {};
}

std::optional<uint8_t> AudioRouter::get_channel_inbound_route(uint8_t source_channel, uint16_t source_uid) {
    for (auto& route : m_inbound_routes) {
        if (route.source_uid == source_uid && route.source_channel == source_channel) {
            return route.local_channel;
        }
    }

    return {};
}

void AudioRouter::route_audio_packet(AudioPacket &packet) {
    const uint8_t source_channel = packet.packet_data.source_channel;
    auto route = get_outbound_route(source_channel);

    if (route.has_value()) {
        send_audio_packet(packet, route->destination_uid);
    }
}

void AudioRouter::create_global_route(RouteNode& source, RouteNode& destination) {
    // Decide which part of the route is created remotely
    if (source.uid == m_self_uid) {
        create_outbound_route(source.channel, destination.uid);
    } else {
        // Create transaction objects for async remote route creation
    }

    if (destination.uid == m_self_uid) {
        create_inbound_route(source.channel, source.uid, destination.channel);
    } else {
        // Create transaction objects for async remote route creation
    }
}

void AudioRouter::clear_all_routes() {
    m_inbound_routes.clear();
    m_outbound_routes.clear();
}

std::optional<RouteNode> AudioRouter::get_inbound_node_from_local_channel(uint8_t local_channel) {
    for (auto& r : m_inbound_routes) {
        if (r.local_channel == local_channel) {
            return RouteNode{r.source_channel, r.source_uid};
        }
    }

    return {};
}
