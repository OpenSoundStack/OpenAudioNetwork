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

#ifndef OPENAUDIONETWORK_NETWORKMAPPER_H
#define OPENAUDIONETWORK_NETWORKMAPPER_H

#include <memory>
#include <cstring>
#include <thread>
#include <unordered_map>
#include <algorithm>
#include <optional>
#include <mutex>

#include "netutils/LowLatSocket.h"
#include "packet_structs.h"

#include "peer/peer_conf.h"

/**
 * @struct PeerInfos
 * @brief Stores other visible devices infos
 */
struct PeerInfos {
    MappingData peer_data;  /**< Device infos */
    uint64_t alive_stamp;   /**< Last alive message timestamp */
};

/**
 * @class NetworkMapper
 * @brief Establishes a network map of all the visible OAN devices in LAN.
 */
class NetworkMapper {
public:
    /**
     * Constructor
     * @param pconf Default self configuration
     */
    NetworkMapper(const PeerConf& pconf);
    ~NetworkMapper();

    /**
     * Network Mapper initialization
     * @param iface Physical network interface name to start the mapping on
     * @return true if initialization succeeds
     */
    bool init_mapper(const std::string& iface);

    /**
     * Launch two threads which scan and map the network
     */
    void launch_mapping_process();

    /**
     * Find a device MAC address based on its UID.
     * @param uid UID to find
     * @return If found, the corresponding MAC address.
     */
    std::optional<uint64_t> get_mac_by_uid(uint16_t uid);

    /**
     * Update self resource mapping
     * @param topo New topology
     */
    void update_resource_mapping(NodeTopology topo);

    /**
     * Update local memory about pipe resource mapping
     * @param topo Temporary deduced topology of peer
     */
    void update_peer_resource_mapping(NodeTopology topo, uint16_t peer_uid);

    /**
     * Finds a DSP Device in the network that has space for new pipes
     * @return If found, DSP ID
     */
    std::optional<uint16_t> find_free_dsp() const;

    /**
     * Finds a free processing channel in a given device
     * @param uid Device to search on
     * @return If found, channel index
     */
    std::optional<uint8_t> first_free_processing_channel(uint16_t uid);

    /**
     * Finds given device topology
     * @param peer_uid Device to search
     * @return If found, device topology
     */
    std::optional<NodeTopology> get_device_topo(uint16_t peer_uid);

    /**
     * Finds all known control surfaces
     * @return The list of known control surfaces
     */
    std::vector<uint16_t> find_all_control_surfaces();

    /**
     * Get the local UNIX time in µs
     * @return Local UNIX time in µs
     */
    static uint64_t local_now();
private:
    void packet_sender();
    void packet_receiver();
    void mapper_process();

    /**
     * Updated the default configuration
     * @param pconf New default configuration
     */
    void update_packet(const PeerConf& pconf);

    void process_packet(MappingPacket pck);

    MappingPacket m_packet;
    uint32_t m_netmask;
    uint16_t m_mapping_port;

    std::unique_ptr<LowLatSocket> m_map_socket;

    std::unordered_map<int, PeerInfos> m_peers;

    std::thread m_tx_thread;
    std::thread m_rx_thread;
    std::thread m_mapper_thread;
    std::mutex m_mapper_mutex;
};


#endif //OPENAUDIONETWORK_NETWORKMAPPER_H
