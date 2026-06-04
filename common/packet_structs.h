// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#ifndef OPENAUDIONETWORK_PACKET_STRUCTS_H
#define OPENAUDIONETWORK_PACKET_STRUCTS_H

#include <cstdint>
#include <vector>
#include <cstring>
#include <string>

#include "audio_conf.h"

#define AUDIO_DATA_SAMPLES_PER_PACKETS 64

/**
 * @enum PacketType
 * @brief Describes packet type in an OANPacket header
 */
enum class PacketType : uint32_t {
    MAPPING = 0x00,     /**< Mapping packet @see MappingData */
    CONTROL = 0x01,     /**< Show control packets */
    CONTROL_CREATE,     /**< Pipe creation packets */
    CONTROL_RESPONSE,   /**< Response to a control command */
    CONTROL_QUERY,      /**< Device request */
    AUDIO,              /**< Audio data packets */
    CLOCK_SYNC,         /**< Time sync between devices */
    UID_PROBE,          /**< Boot-time UID claim probe @see UidProbeData */
    UID_DEFEND          /**< Incumbent defends its committed UID @see UidDefendData */
};

/**
 * @enum ClockType
 * @brief Describes clock type of aa device
 */
enum ClockType : uint32_t {
    CKTYPE_NONE = 0,    /** No clock sync on device */
    CKTYPE_MASTER = 1,  /** Device is a clock master */
    CKTYPE_SLAVE = 2    /** Device is a clock save */
};

/**
 * @enum DeviceType
 * @brief Describes the current device type in mapping packets
 */
enum class DeviceType : uint16_t {
    CONTROL_SURFACE,    /**< Control surface device with only control and GUI */
    MONITORING,         /**< Monitoring interface that receives only audio and do not send/receive control packets */
    AUDIO_IO_INTERFACE, /**< Physical IO interface */
    AUDIO_DSP           /**< Audio signal processor */
};

/**
 * @enum ControlQueryType
 * @brief Defines the query types you can send to a device
 */
enum class ControlQueryType : uint32_t {
    PHY_OUT_MAP,        /**< Get the map of free physical out in the device */
    PIPES_MAP,          /**< Get the map of free processing pipes in the device */
    PIPE_ALLOC_RESET,   /**< Reset pipes and channel allocator in device */
    SET_AUDIO_DEST,     /**< Source peer: stamp this dest_uid into outgoing audio. dest_uid carried in response[0] low 16 bits. */
    CLEAR_AUDIO_DEST,   /**< Source peer: stop sending audio. */
    SET_INPUT_ROUTE,    /**< Target peer: when audio arrives from src_uid with packet ch=src_ch, feed it into local pipe dest_pipe. response[0]=src_uid|(src_ch<<16), response[1]=dest_pipe. */
    CLEAR_INPUT_ROUTE,  /**< Target peer: drop the route entry on local pipe dest_pipe. response[1]=dest_pipe. */
};

/**
 * @enum DataTypes
 * Data type enum for the ControlPacket
 */
enum class DataTypes : uint8_t {
    INT8,
    INT16,
    INT32,
    FLOAT,
    CUSTOM
};

/**
 * @enum ControlResponseCode
 * @brief Error codes for control packets that needs a response
 */
enum ControlResponseCode : uint8_t {
    CREATE_OK = 1 << 0,
    CREATE_ERROR = 1 << 1,
    CONTROL_ACK = 1 << 2,
    CREATE_TYPE_UNK = 1 << 3,
    CREATE_ALLOC_FAILED = 1 << 4, /**< Resource allocation failed on pipe creation */
    CONTROL_ERROR,                /**< Control generic error */
};

/**
 * Utils function that encode version numbers for packets
 * @tparam maj Major version
 * @tparam min Minor version
 * @return Encoded version number
 */
template<uint32_t maj, uint32_t min>
constexpr uint32_t make_version_number() {
    return (maj << 16) | (min & 0xFFFF);
}

/**
 * @struct NodeTopology
 * @brief Describe each node physical and compute capacity
 */
struct NodeTopology {
    uint8_t phy_in_count;   /**< Physical input count */
    uint8_t phy_out_count;  /**< Physical output count */
    uint8_t pipes_count;    /**< Available processing pipes */

    uint64_t phy_out_resmap;
    uint64_t pipe_resmap;
    uint8_t __padding__;
} __attribute__((packed));

/**
 * @struct CommonHeader
 * @brief Common OANPacket header
 */
struct CommonHeader {
    PacketType type;        /**< Encapsulated packet type */
    uint16_t version;       /**< Protocol version */
    uint16_t flags;         /**< Header flags (currently unused) */
    uint64_t timestamp;     /**< Packet timestamp, used only for audio packets */
    uint64_t prev_delay;    /**< Previous accumulated delay in us */
} __attribute__((packed));

/**
 * @struct OANPacket
 * @brief Constructs a valid OANPacket with some encapsulated data and a header
 * @tparam Tdata Data type to encapsulate
 */
template<class Tdata>
struct OANPacket {
    CommonHeader header;    /**< Packet header */
    Tdata packet_data;      /**< Encapsulated data */
};

/**
 * @struct MappingData
 * @brief Packet type used to describe OAN compatible-devices on the network
 */
struct MappingData {
    char dev_name[32];         /**< Device name */
    uint64_t self_address;     /**< Device MAC address */
    uint16_t self_uid;         /**< Device UID */
    DeviceType type;           /**< Device Type */
    SamplingRate sample_rate;  /**< Device sample rate (IO & DSP only) */
    NodeTopology topo;         /**< Device physical and compute topology */
    ClockType ck_type;         /**< Device clock type */
};

/**
 * @struct ControlPipeCreate
 * @brief Pipe element creation packet
 */
struct ControlPipeCreate {
    uint8_t channel;          /**<  Channel to alter */
    uint8_t stack_position;   /**< Position in pipe */
    uint8_t seq;              /**< Sequence number */
    uint8_t seq_max;          /**< Expected packet count for the whole creation instruction */
    uint16_t pid;             /**< Pipe ID */
    char elem_type[32];       /**< Element type name to create */
};

/**
 * @struct ControlData
 * @brief Pipe control packet type.
 * Control 0 is reserved for channel level data
 */
struct ControlData {
    uint8_t channel;          /**< Channel of the pipe */
    uint8_t elem_index;       /**< Element index in pipe */
    uint16_t control_id;      /**< Control ID in the pipe */
    DataTypes control_type;   /**< Control data type */
    uint32_t data[4];         /**< Control data */
};

/**
 * @struct ControlResponse
 * @brief Error management in control frames. Especially for pipe creation.
 */
struct ControlResponse {
    uint8_t response;             /**< Response code */
    uint8_t channel;              /**< Channel affected */
    uint16_t pid;                 /**< Pipe/Transaction ID */
    char err_msg[64];             /**< Error message */
};

/**
 * @struct ControlQuery
 * @brief Query useful infos to any device.
 *
 * @note Despite the field name, @c response is also used as the
 * request-side payload slot for newer query types — see the
 * dynamic-routing proposal (Docs/proposals/dynamic-routing.md) where
 * SET_AUDIO_DEST / SET_INPUT_ROUTE pack their arguments into
 * response[0..1]. The interpretation of @c response is per-qtype.
 */
struct ControlQuery {
    ControlQueryType qtype;     /**< Type of the query */
    uint32_t flags;             /**< Additional flags */
    uint32_t response[4];       /**< Per-qtype payload (request and/or response) */
};

/**
 * @struct AudioData
 * @brief Packet containing audio samples
 */
struct AudioData {
    uint8_t source_channel;                         /**< If packet sent from another pipe, specify source */
    uint8_t channel;                                /**< Channel transported */
    float samples[AUDIO_DATA_SAMPLES_PER_PACKETS];  /**< Sample data */
};

/**
 * @struct ClockSync
 * @brief As the timestamp is already contained in the packet header we only have to notify the clock sync state (PTP states)
 */
struct ClockSync {
    uint8_t packet_state;
};

/**
 * @struct UidProbeData
 * @brief Boot-time probe asking the segment "is anyone using this UID?".
 *
 * Sent broadcast on the discovery EtherType (0x0682). The candidate UID
 * is not yet committed; receivers MUST NOT enter it into their peer
 * table on the basis of a probe alone.
 */
struct UidProbeData {
    uint16_t candidate_uid;     /**< UID the sender is trying to claim */
    uint64_t src_mac;           /**< Sender MAC, low 48 bits */
    uint8_t  salt;              /**< Diagnostic only; not authoritative */
    uint8_t  __padding__[5];
} __attribute__((packed));

/**
 * @struct UidDefendData
 * @brief Incumbent's response to a probe or to a peer claiming an in-use UID.
 *
 * Sent broadcast. Defender-wins rule: the incumbent NEVER renumbers in
 * response to a runtime collision; the newcomer salt-walks instead.
 */
struct UidDefendData {
    uint16_t defended_uid;      /**< UID the sender currently holds */
    uint64_t src_mac;           /**< Sender (incumbent) MAC, low 48 bits */
    uint64_t since_us;          /**< Local timestamp of original commit; diagnostic */
} __attribute__((packed));

typedef OANPacket<MappingData> MappingPacket;                   /**< Full OAN Packet for mapping data */
typedef OANPacket<AudioData> AudioPacket;                       /**< Full OAN Packet for audio data */
typedef OANPacket<ControlData> ControlPacket;                   /**< Full OAN Packet for control data */
typedef OANPacket<ControlPipeCreate> ControlPipeCreatePacket;   /**< Full OAN Packet for pipe creation */
typedef OANPacket<ControlResponse> ControlResponsePacket;       /**< Full OAN Packet for control response */
typedef OANPacket<ControlQuery> ControlQueryPacket;             /**< Full OAN Packet for control query */
typedef OANPacket<ClockSync> ClockSyncPacket;                   /**< Full OAN Packet for clock synchronization between devices */
typedef OANPacket<UidProbeData> UidProbePacket;                 /**< Full OAN Packet for UID probe */
typedef OANPacket<UidDefendData> UidDefendPacket;               /**< Full OAN Packet for UID defend */

#endif //OPENAUDIONETWORK_PACKET_STRUCTS_H
