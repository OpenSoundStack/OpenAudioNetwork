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

#include "ClockSlave.h"

ClockSlave::ClockSlave(uint16_t self_uid, const std::string &iface, std::shared_ptr<NetworkMapper> nmapper) {
    for (auto& t : m_tstamps) {
        t = 0;
    }

    m_sync_socket = std::make_unique<LowLatSocket>(self_uid, nmapper);
    if (!m_sync_socket->init_socket(iface, EthProtocol::ETH_PROTO_OANSYNC)) {
#ifdef __linux__
        std::cerr << "Failed to init sync iface" << std::endl;
#endif // __linux__
    }

    m_nmapper = nmapper;
}

void ClockSlave::sync_process() {
    LowLatPacket<ClockSyncPacket> pck{};
    if (m_sync_socket->receive_data(&pck) <= 0) {
        return;
    }

    if (pck.payload.header.type == PacketType::CLOCK_SYNC) {
        switch (pck.payload.packet_data.packet_state) {
            case ClockSyncState::CKSYNC_SYNC:
                m_tstamps[0] = pck.payload.header.timestamp;  // t1
                m_tstamps[1] = NetworkMapper::local_now_us(); // t2
                send_delay_req(pck.llhdr.sender_uid);
                break;
            case ClockSyncState::CKSYNC_DELAY_RESP:
                m_tstamps[3] = pck.payload.header.timestamp; // t4
                calc_ck_offset();
                break;
            default:
                break;
        }
    }
}

void ClockSlave::send_delay_req(uint16_t dest) {
    ClockSyncPacket pck{};
    pck.header.type = PacketType::CLOCK_SYNC;
    pck.header.timestamp = NetworkMapper::local_now_us();
    pck.packet_data.packet_state = ClockSyncState::CKSYNC_DELAY_REQ;

    m_sync_socket->send_data(pck, dest);
    m_tstamps[2] = pck.header.timestamp; // t3
}

void ClockSlave::calc_ck_offset() {
    int64_t delay1 = m_tstamps[1] - m_tstamps[0];
    int64_t delay2 = m_tstamps[3] - m_tstamps[2];

    m_ck_offset = (delay1 - delay2) / 2;
}

int64_t ClockSlave::get_ck_offset() {
    return m_ck_offset;
}
