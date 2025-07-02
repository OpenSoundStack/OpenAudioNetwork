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

#ifndef CLOCKMASTER_H
#define CLOCKMASTER_H

#include "netutils/LowLatSocket.h"
#include "NetworkMapper.h"
#include "clock.h"

#include <unordered_map>

class ClockMaster {
public:
    ClockMaster(uint16_t self_uid, const std::string& iface, std::shared_ptr<NetworkMapper> nmapper);
    ~ClockMaster() = default;

    void begin_sync_process();
    void sync_process();

private:
    void start_clock_sync(PeerInfos& slave);
    void process_packet(ClockSyncPacket& csp, uint16_t originator);

    std::shared_ptr<NetworkMapper> m_nmapper;
    std::shared_ptr<LowLatSocket> m_sync_socket;

    std::unordered_map<uint16_t, ClockSyncState> m_sync_states;
};



#endif //CLOCKMASTER_H
