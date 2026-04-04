// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#ifndef CLOCKSLAVE_H
#define CLOCKSLAVE_H

#include "NetworkMapper.h"
#include "netutils/LowLatSocket.h"
#include "clock.h"

class ClockSlave {
public:
    ClockSlave(uint16_t self_uid, const std::string& iface, std::shared_ptr<NetworkMapper> nmapper);
    ~ClockSlave() = default;

    void sync_process();
    int64_t get_ck_offset();

private:
    void send_delay_req(uint16_t dest);
    void calc_ck_offset();

    std::shared_ptr<NetworkMapper> m_nmapper;
    std::unique_ptr<LowLatSocket> m_sync_socket;

    uint64_t m_tstamps[4];
    int64_t m_ck_offset;
};



#endif //CLOCKSLAVE_H
