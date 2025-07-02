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

//
// Created by mathis on 30/06/25.
//

#include "ClockMaster.h"

ClockMaster::ClockMaster(uint16_t self_uid, const std::string& iface, std::shared_ptr<NetworkMapper> nmapper) {
    m_nmapper = nmapper;

    m_sync_socket = std::make_shared<LowLatSocket>(self_uid, nmapper);
    if (!m_sync_socket->init_socket(iface, EthProtocol::ETH_PROTO_OANSYNC)) {
        std::cerr << "Failed to init clock sync socket !" << std::endl;
    }

    m_sync_states = {};
}

void ClockMaster::begin_sync_process() {
    m_sync_states.clear();

    auto slaves = m_nmapper->get_clock_slaves();
    for (auto& s : slaves) {
        start_clock_sync(s);
    }
}

void ClockMaster::sync_process() {
    LowLatPacket<ClockSyncPacket> ck_packet{};

    if (m_sync_socket->receive_data(&ck_packet) > 0) {
        if (ck_packet.payload.header.type == PacketType::CLOCK_SYNC) {
            process_packet(ck_packet.payload, ck_packet.llhdr.sender_uid);
        }
    }
}

void ClockMaster::process_packet(ClockSyncPacket &csp, uint16_t originator) {
    if (csp.packet_data.packet_state == ClockSyncState::CKSYNC_DELAY_REQ) {
        ClockSyncPacket del_resp{};
        del_resp.header.type = PacketType::CLOCK_SYNC;
        del_resp.header.timestamp = NetworkMapper::local_now_us();
        del_resp.packet_data.packet_state = ClockSyncState::CKSYNC_DELAY_RESP;

        m_sync_socket->send_data(del_resp, originator);
    }
}

void ClockMaster::start_clock_sync(PeerInfos &slave) {
    ClockSyncPacket pck{};
    pck.header.type = PacketType::CLOCK_SYNC;
    pck.header.timestamp = NetworkMapper::local_now_us();
    pck.packet_data.packet_state = ClockSyncState::CKSYNC_SYNC;

    m_sync_socket->send_data(pck, slave.peer_data.self_uid);

    m_sync_states[slave.peer_data.self_uid] = ClockSyncState::CKSYNC_SYNC;
}

