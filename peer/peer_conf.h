// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#ifndef OPENAUDIONETWORK_PEER_CONF_H
#define OPENAUDIONETWORK_PEER_CONF_H

#include <cstdint>
#include <string>
#include "common/packet_structs.h"

struct PeerConf {
    char dev_name[32];
    std::string iface;
    uint16_t uid;
    DeviceType dev_type;
    SamplingRate sample_rate;
    NodeTopology topo;
    ClockType ck_type;
};

#endif //OPENAUDIONETWORK_PEER_CONF_H
