// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#include "NetworkMapper.h"

#if defined(__linux__) || defined(OAN_HOST_BACKENDS)
#include <unistd.h>
#endif

#if !defined(__linux__) && !defined(OAN_HOST_BACKENDS)
extern uint32_t _now_ms();
extern uint64_t _now_us();
extern "C" IfaceMeta _fetch_iface_meta(const std::string&);
#endif

NetworkMapper::NetworkMapper(const PeerConf& pconf) {
    m_peer_change_callback = [](PeerInfos&, bool) {};
    update_packet(pconf);
}

NetworkMapper::~NetworkMapper() {

}

bool NetworkMapper::init_mapper(const std::string& iface) {
    m_map_socket = std::make_unique<LowLatSocket>(m_packet.packet_data.self_uid, std::shared_ptr<NetworkMapper>{});
    bool res = m_map_socket->init_socket(iface, EthProtocol::ETH_PROTO_OANDISCO);
    if (!res) return false;

#ifdef OAN_HOST_BACKENDS
    // On the host-backend path, the transport (sim/raw-mac) supplies the
    // device MAC — there is no real interface to query. The MAC in
    // m_packet was zero-filled by update_packet() at construction; fill it
    // now from the socket whose transport just learned it.
    m_packet.packet_data.self_address = 0;
    memcpy(&m_packet.packet_data.self_address, m_map_socket->get_self_mac(), 6);
#endif

    return true;
}

void NetworkMapper::update_packet(const PeerConf &pconf) {
#if defined(__linux__)
    auto iface_meta = get_iface_meta(pconf.iface);
#elif defined(OAN_HOST_BACKENDS)
    // Skip the iface lookup entirely: pconf.iface may be a transport prefix
    // (sim:default, raw:en0) for which no real OS-level lookup makes sense.
    IfaceMeta iface_meta{};
    // If we already have a socket (i.e. update_packet is being re-run after
    // init_mapper), preserve the MAC the transport learned. Otherwise leave
    // zero and let init_mapper fill it in once the socket is bound.
    if (m_map_socket) {
        memcpy(iface_meta.mac, m_map_socket->get_self_mac(), 6);
    }
#else
    auto iface_meta = _fetch_iface_meta(pconf.iface);
#endif

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
#ifndef NO_THREADS
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
#endif // NO_THREADS
}

void NetworkMapper::mapper_update() {
    uint64_t now = local_now();
    constexpr int die_timeout = 15000;

    std::erase_if(m_peers, [now, this](const std::pair<int, PeerInfos> &pred) {
        uint64_t delta = now - pred.second.alive_stamp;
        if (delta > die_timeout) {
            PeerInfos pinfo = pred.second;

#ifdef __linux__
            std::cout << "Lost " << pinfo.peer_data.dev_name << " (ID = " << pinfo.peer_data.self_uid << ")" << std::endl;
#endif // __linux__

            std::erase_if(m_ck_slaves, [this, pred](const PeerInfos& pi) {
                return pi.peer_data.self_uid == pred.second.peer_data.self_uid;
            });

            m_peer_change_callback(pinfo, false);

            return true;
        }

        return false;
    });
}

void NetworkMapper::packet_recv_update() {
    LowLatPacket<MappingPacket> pck{};
    int rx_data = m_map_socket->receive_data(&pck, false);

    if(rx_data > 0 && pck.payload.header.type == PacketType::MAPPING) {
        process_packet(pck.payload);
    }
}

void NetworkMapper::packet_send_update() {
    m_map_socket->send_data<MappingPacket>(m_packet, 0);
}

void NetworkMapper::packet_sender() {
    while(true) {
        packet_send_update();
#if defined(__linux__) || defined(OAN_HOST_BACKENDS)
        usleep(5000000);
#endif
    }
}

void NetworkMapper::packet_receiver() {
    while(true) {
        packet_recv_update();
    }
}

void NetworkMapper::mapper_process() {
    while(true) {
        {
#ifndef NO_THREADS
            std::lock_guard<std::mutex> m{m_mapper_mutex};
#endif // NO_THREADS
            mapper_update();
        }

#if defined(__linux__) || defined(OAN_HOST_BACKENDS)
        usleep(1000000);
#endif
    }
}

void NetworkMapper::process_packet(MappingPacket pck) {
    uint64_t now = local_now();

    PeerInfos pinfo = {};
    memcpy(&pinfo.peer_data, &pck.packet_data, sizeof(MappingData));
    pinfo.alive_stamp = now;

    if(!m_peers.contains(pck.packet_data.self_uid)) {
#ifdef __linux__
        std::cout << "Discovered " << pck.packet_data.dev_name << " (ID = " << pck.packet_data.self_uid << ")" << std::endl;
        std::cout << "Audio config : " << (int)pck.packet_data.topo.phy_out_count << " outs" << std::endl;
        std::cout << "               " << (int)pck.packet_data.topo.phy_in_count << " ins" << std::endl;
        std::cout << "               " << (int)pck.packet_data.topo.pipes_count << " pipes" << std::endl;
#endif // __linux__

        {
#ifndef NO_THREADS
            std::lock_guard<std::mutex> m{m_mapper_mutex};
#endif // NO_THREADS
            m_peers[pck.packet_data.self_uid] = pinfo;

            if (pinfo.peer_data.ck_type == CKTYPE_SLAVE) {
#ifdef __linux__
                std::cout << "New clock slave" << std::endl;
#endif // __linux__
                m_ck_slaves.emplace_back(pinfo);
            }
        }

        m_peer_change_callback(pinfo, true);
        m_temp_peers.erase(pck.packet_data.self_uid); // In case there was a temp peer associated with that ID, remove it
    } else {
        {
#ifndef NO_THREADS
            std::lock_guard<std::mutex> m{m_mapper_mutex};
#endif // NO_THREADS
            m_peers[pck.packet_data.self_uid] = pinfo;
        }
    }
}

std::optional<uint64_t> NetworkMapper::get_mac_by_uid(uint16_t uid) {
    if (m_peers.contains(uid)) {
        return m_peers[uid].peer_data.self_address;
    } else if (m_temp_peers.contains(uid)) {
        return m_temp_peers[uid].peer_data.self_address;
    } else {
        return {};
    }
}

void NetworkMapper::update_peer_resource_mapping(NodeTopology topo, uint16_t peer_uid) {
    if (m_peers.contains(peer_uid)) {
#ifndef NO_THREADS
        std::lock_guard<std::mutex> m{m_mapper_mutex};
#endif // NO_THREADS
        auto peer_infos = m_peers[peer_uid];
        peer_infos.peer_data.topo = topo;
    } else {
        return;
    }
}

void NetworkMapper::update_resource_mapping(NodeTopology topo) {
#ifndef NO_THREADS
    std::lock_guard<std::mutex> m{m_mapper_mutex};
#endif // NO_THREADS
    m_packet.packet_data.topo = topo;
}

std::optional<uint16_t> NetworkMapper::find_free_dsp() const {
    for (auto& peer : m_peers) {
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
#if defined(__linux__) || defined(__ZEPHYR__) || defined(OAN_HOST_BACKENDS)
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
#else
    return _now_ms();
#endif
}

uint64_t NetworkMapper::local_now_us() {
#if defined(__linux__) || defined(__ZEPHYR__) || defined(OAN_HOST_BACKENDS)
    return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
#else
    return _now_us();
#endif
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

void NetworkMapper::add_temp_peer(uint16_t uid, const PeerInfos &infos) {
    m_temp_peers[uid] = std::move(infos);
}
