#include "NetworkMapper.h"

NetworkMapper::NetworkMapper(const PeerConf& pconf) {
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
            std::erase_if(m_peers, [now](const std::pair<int, PeerInfos> &pred) {
                uint64_t delta = now - pred.second.alive_stamp;
                if (delta > die_timeout) {
                    std::cout << "Lost " << pred.second.peer_data.dev_name << " (ID = "
                              << pred.second.peer_data.self_uid << ")" << std::endl;
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

        {
            std::lock_guard<std::mutex> m{m_mapper_mutex};
            m_peers[pck.packet_data.self_uid] = {pck.packet_data, now};
        }
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

std::optional<NodeTopology> NetworkMapper::get_device_topo(uint16_t peer_uid) {
    if (m_peers.contains(peer_uid)) {
        return m_peers[peer_uid].peer_data.topo;
    } else {
        return {};
    }
}

