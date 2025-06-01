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
};

#endif //OPENAUDIONETWORK_PEER_CONF_H
