#ifndef OPENAUDIONETWORK_PACKET_STRUCTS_H
#define OPENAUDIONETWORK_PACKET_STRUCTS_H

#include <cstdint>
#include <vector>
#include <cstring>
#include <string>

#include "audio_conf.h"

#define AUDIO_DATA_SAMPLES_PER_PACKETS 64

enum class PacketType : uint32_t {
    MAPPING,
    CONTROL,
    CONTROL_CREATE,
    AUDIO
};

enum class DeviceType : uint32_t {
    CONTROL_SURFACE,
    MONITORING,
    AUDIO_IO_INTERFACE,
    AUDIO_DSP
};

enum class DataTypes : uint8_t {
    INT8,
    INT16,
    INT32,
    FLOAT
};

enum class ControlResponseCode : uint8_t {
    CREATE_OK,
    CREATE_ERROR,
    CONTROL_ACK
};

template<uint32_t maj, uint32_t min>
constexpr uint32_t make_version_number() {
    return (maj << 16) | (min & 0xFFFF);
}

struct NodeTopology {
    uint8_t phy_in_count;
    uint8_t phy_out_count;
    uint8_t pipes_count;
};

struct CommonHeader {
    PacketType type;
    uint16_t version;
    uint16_t flags;
    uint64_t timestamp;
};

template<class Tdata>
struct OANPacket {
    CommonHeader header;
    Tdata packet_data;
};

struct MappingData {
    char dev_name[32];
    uint64_t self_address;
    uint16_t self_uid;
    DeviceType type;
    SamplingRate sample_rate;
    NodeTopology topo;
};

struct ControlPipeCreate {
    uint8_t channel;
    uint8_t stack_position;
    uint8_t seq;
    char elem_type[32];
};

struct ControlData {
    uint8_t channel;
    uint16_t control_id;
    DataTypes control_type;
    uint32_t data[4];
};

struct ControlResponse {
    ControlResponseCode response;
    uint8_t seq;
    char err_msg[64];
};

struct AudioData {
    uint8_t channel;
    float samples[AUDIO_DATA_SAMPLES_PER_PACKETS];
};

typedef OANPacket<MappingData> MappingPacket;
typedef OANPacket<AudioData> AudioPacket;
typedef OANPacket<ControlData> ControlPacket;
typedef OANPacket<ControlPipeCreate> ControlPipeCreatePacket;
typedef OANPacket<ControlResponseCode> ControlResponsePacket;

#endif //OPENAUDIONETWORK_PACKET_STRUCTS_H
