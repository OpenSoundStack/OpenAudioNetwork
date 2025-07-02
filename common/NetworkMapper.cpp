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

#include "NetworkMapper.h"

NetworkMapper::NetworkMapper(const PeerConf& pconf) {
    m_peer_change_callback = [](PeerInfos&, bool) {};
    update_packet(pconf);
}

NetworkMapper::~NetworkMapper() {

}

bool NetworkMapper::init_mapper(const std::string& iface) {
    m_map_socket = std::make_unique<LowLatSocket>(m_packet.packet_data.self_uid, std::shared_ptr<NetworkMapper>{});
    bool res = m_map_socket->init_socket(iface, EthProtocol::ETH_PROTO_OANDISCO);

    return res;
}

void NetworkMapper::update_packet(const PeerConf &pconf) {
    auto iface_meta = get_iface_meta(pconf.iface);

    m_packet.header.type = PacketType::MAPPING;
    m_packet.packet_data.topo = pconf.topo;

    m_packet.packet_data.self_uid = pconf.uid;
    m_packet.packet_data.type = pconf.dev_type;
    m_packet.packet_data.sample_rate = pconf.sample_rate;

    m_packet.packet_data.ck_type = pconf.ck_type;

    m_packet.packet_data.self_address = 0;
    memcpy(&m_packet.packet_data.self_address, iface_meta.mac, 6);

    memcpy(&m_packet.packet_data.dev_name, pconf.dev_name, 32);
}

void NetworkMapper::launch_mapping_process() {
    // Mapper sending thread
    m_tx_thread = std::thread([this]() {
        packet_sender();
    });
    m_tx_thread.detach();

    // Mapper receiving thread
    m_rx_thread = std::thread([this]() {
        packet_receiver();
    });
    m_rx_thread.detach();

    m_mapper_thread = std::thread([this]() {
        mapper_process();
    });
    m_mapper_thread.detach();
}

void NetworkMapper::packet_sender() {
    while(true) {
        m_map_socket->send_data<MappingPacket>(m_packet, 0);

        usleep(5000000);
    }
}

void NetworkMapper::packet_receiver() {
    LowLatPacket<MappingPacket> pck{};
    sockaddr_in sender = {};

    while(true) {
        m_map_socket->receive_data(&pck, false);

        if(pck.payload.header.type == PacketType::MAPPING) {
            process_packet(pck.payload);
        }
    }
}

void NetworkMapper::mapper_process() {
    constexpr int die_timeout = 15000;

    while(true) {
        uint64_t now = local_now();

        {
            std::lock_guard<std::mutex> m{m_mapper_mutex};
            std::erase_if(m_peers, [now, this](const std::pair<int, PeerInfos> &pred) {
                uint64_t delta = now - pred.second.alive_stamp;
                if (delta > die_timeout) {
                    PeerInfos pinfo = pred.second;

                    std::cout << "Lost " << pinfo.peer_data.dev_name << " (ID = "
                                                  << pinfo.peer_data.self_uid << ")" << std::endl;

                    std::erase_if(m_ck_slaves, [this, pred](const PeerInfos& pi) {
                        return pi.peer_data.self_uid == pred.second.peer_data.self_uid;
                    });

                    m_peer_change_callback(pinfo, false);

                    return true;
                }

                return false;
            });
        }

        usleep(1000000);
    }
}

void NetworkMapper::process_packet(MappingPacket pck) {
    uint64_t now = local_now();

    if(!m_peers.contains(pck.packet_data.self_uid)) {
        std::cout << "Discovered " << pck.packet_data.dev_name << " (ID = " << pck.packet_data.self_uid << ")" << std::endl;
        std::cout << "Audio config : " << (int)pck.packet_data.topo.phy_out_count << " outs" << std::endl;
        std::cout << "               " << (int)pck.packet_data.topo.phy_in_count << " ins" << std::endl;
        std::cout << "               " << (int)pck.packet_data.topo.pipes_count << " pipes" << std::endl;

        PeerInfos pinfo = {pck.packet_data, now};

        {
            std::lock_guard<std::mutex> m{m_mapper_mutex};
            m_peers[pck.packet_data.self_uid] = pinfo;

            if (pinfo.peer_data.ck_type == CKTYPE_SLAVE) {
                std::cout << "New clock slave" << std::endl;
                m_ck_slaves.emplace_back(pinfo);
            }
        }

        m_peer_change_callback(pinfo, true);
    } else {
        {
            std::lock_guard<std::mutex> m{m_mapper_mutex};
            m_peers[pck.packet_data.self_uid] = {pck.packet_data, now};
        }
    }
}

std::optional<uint64_t> NetworkMapper::get_mac_by_uid(uint16_t uid) {
    if (m_peers.contains(uid)) {
        return m_peers[uid].peer_data.self_address;
    } else {
        return {};
    }
}

void NetworkMapper::update_peer_resource_mapping(NodeTopology topo, uint16_t peer_uid) {
    if (m_peers.contains(peer_uid)) {
        std::lock_guard<std::mutex> m{m_mapper_mutex};
        auto peer_infos = m_peers[peer_uid];
        peer_infos.peer_data.topo = topo;
    } else {
        return;
    }
}

void NetworkMapper::update_resource_mapping(NodeTopology topo) {
    std::lock_guard<std::mutex> m{m_mapper_mutex};
    m_packet.packet_data.topo = topo;
}

std::optional<uint16_t> NetworkMapper::find_free_dsp() const {
    for (auto peer : m_peers) {
        PeerInfos infos = peer.second;

        // Each bit in the resource map represent a channel, 1 if used, 0 if not. If the pipe resource map is 0
        // then there are no free channels
        if (infos.peer_data.type == DeviceType::AUDIO_DSP && infos.peer_data.topo.pipe_resmap != 0) {
            return infos.peer_data.self_uid;
        }
    }

    return {};
}

std::optional<uint8_t> NetworkMapper::first_free_processing_channel(uint16_t uid) {
    if (m_peers.contains(uid)) {
        PeerInfos infos = m_peers[uid];

        uint64_t resmap = infos.peer_data.topo.pipe_resmap;
        for (int i = 0; i < 64; i++) {
            if (resmap & 0x01) {
                return i;
            }

            resmap >>= 1;
        }
    }

    return {};
}

uint64_t NetworkMapper::local_now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

uint64_t NetworkMapper::local_now_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

std::optional<NodeTopology> NetworkMapper::get_device_topo(uint16_t peer_uid) {
    if (m_peers.contains(peer_uid)) {
        return m_peers[peer_uid].peer_data.topo;
    } else {
        return {};
    }
}

std::vector<uint16_t> NetworkMapper::find_all_control_surfaces() {
    std::vector<uint16_t> surfaces;

    for (auto& device : m_peers) {
        if (device.second.peer_data.type == DeviceType::CONTROL_SURFACE) {
            surfaces.push_back(device.first);
        }
    }

    return surfaces;
}

void NetworkMapper::set_peer_change_callback(std::function<void(PeerInfos &, bool)> callback) {
    m_peer_change_callback = std::move(callback);
}

std::vector<PeerInfos> NetworkMapper::get_clock_slaves() {
    return m_ck_slaves;
}
