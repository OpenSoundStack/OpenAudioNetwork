#ifndef OPENAUDIONETWORK_PEER_CONF_H
#define OPENAUDIONETWORK_PEER_CONF_H

#include <cstdint>
#include "common/packet_structs.h"

struct PeerConf {
    uint32_t address;
    uint32_t netmask;
    uint16_t mapping_port;
    uint16_t audio_port;
    uint16_t uid;
    DeviceType dev_type;
    char dev_name[32];
    SamplingRate sample_rate;
    NodeTopology topo;
};

#endif //OPENAUDIONETWORK_PEER_CONF_H
