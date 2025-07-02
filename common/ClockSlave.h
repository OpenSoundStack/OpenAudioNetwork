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
