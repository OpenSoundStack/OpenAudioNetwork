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
