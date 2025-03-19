#ifndef OPENAUDIONETWORK_NETWORKMAPPER_H
#define OPENAUDIONETWORK_NETWORKMAPPER_H

#include <memory>
#include <cstring>
#include <thread>
#include <unordered_map>
#include <algorithm>

#include "../netutils/udp.h"
#include "packet_structs.h"

#include "../peer/peer_conf.h"

struct PeerInfos {
    MappingData peer_data;
    uint64_t alive_stamp;
};

class NetworkMapper {
public:
    NetworkMapper(const PeerConf& pconf);
    ~NetworkMapper();

    bool init_mapper();
    void launch_mapping_process();

private:
    void packet_sender();
    void packet_receiver();
    void mapper_process();

    void update_packet(const PeerConf& pconf);

    void process_packet(MappingPacket pck);

    static uint64_t local_now();

    MappingPacket m_packet;
    uint32_t m_netmask;
    uint16_t m_mapping_port;

    std::unique_ptr<UDPSocket> m_map_socket;

    std::unordered_map<int, PeerInfos> m_peers;

    std::thread m_tx_thread;
    std::thread m_rx_thread;
    std::thread m_mapper_thread;
    std::mutex m_mapper_mutex;
};


#endif //OPENAUDIONETWORK_NETWORKMAPPER_H
