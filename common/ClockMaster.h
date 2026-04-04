// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

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
