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
    MAPPING,            /**< Mapping packet @see MappingData */
    CONTROL,            /**< Show control packets */
    CONTROL_CREATE,     /**< Pipe creation packets */
    CONTROL_RESPONSE,   /**< Response to a control command */
    AUDIO               /**< Audio data packets */
};

/**
 * @enum DeviceType
 * @brief Describes the current device type in mapping packets
 */
enum class DeviceType : uint32_t {
    CONTROL_SURFACE,    /**< Control surface device with only control and GUI */
    MONITORING,         /**< Monitoring interface that receives only audio and do not send/receive control packets */
    AUDIO_IO_INTERFACE, /**< Physical IO interface */
    AUDIO_DSP           /**< Audio signal processor */
};

/**
 * @enum DataTypes
 * Data type enum for the ControlPacket
 */
enum class DataTypes : uint8_t {
    INT8,
    INT16,
    INT32,
    FLOAT
};

/**
 * @enum ControlResponseCode
 * @brief Error codes for control packets that needs a response
 */
enum ControlResponseCode : uint8_t {
    CREATE_OK = 1 << 0,
    CREATE_ERROR = 1 << 1,
    CONTROL_ACK = 1 << 2,
    CREATE_TYPE_UNK = 1 << 3
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
};

/**
 * @struct CommonHeader
 * @brief Common OANPacket header
 */
struct CommonHeader {
    PacketType type;        /**< Encapsulated packet type */
    uint16_t version;       /**< Protocol version */
    uint16_t flags;         /**< Header flags (currently unused) */
    uint64_t timestamp;     /**< Packet timestamp, used only for audio packets */
};

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
    char elem_type[32];       /**< Element type name to create */
};

/**
 * @struct ControlData
 * @brief Pipe control packet type.
 */
struct ControlData {
    uint8_t channel;          /**< Channel of the pipe */
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
    char err_msg[64];             /**< Error message */
};

/**
 * @struct AudioData
 * @brief Packet containing audio samples
 */
struct AudioData {
    uint8_t channel;                                /**< Channel transported */
    float samples[AUDIO_DATA_SAMPLES_PER_PACKETS];  /**< Sample data */
};

typedef OANPacket<MappingData> MappingPacket;                   /**< Full OAN Packet for mapping data */
typedef OANPacket<AudioData> AudioPacket;                       /**< Full OAN Packet for audio data */
typedef OANPacket<ControlData> ControlPacket;                   /**< Full OAN Packet for control data */
typedef OANPacket<ControlPipeCreate> ControlPipeCreatePacket;   /**< Full OAN Packet for pipe creation */
typedef OANPacket<ControlResponse> ControlResponsePacket;       /**< Full OAN Packet for control response */

#endif //OPENAUDIONETWORK_PACKET_STRUCTS_H
