// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#ifndef LLSCOMMON_H
#define LLSCOMMON_H

#include <cstdint>

#ifndef __linux__
// Defining linux-like structs for compat
struct EthernetHeader {
    uint8_t h_dest[6];
    uint8_t h_source[6];
    uint16_t h_proto;
} __attribute__((packed));
typedef EthernetHeader ethhdr;
#else
#include <linux/if_ether.h>
#endif // __linux__


/**
 * @enum EthProtocol
 * @brief Enum containing different values for the Ethernet protocol field, corresponding to different OAN stream types.
 */
enum EthProtocol : uint16_t {
    ETH_PROTO_OANAUDIO = 0x0681,
    ETH_PROTO_OANDISCO = 0x0682,
    ETH_PROTO_OANCONTROL = 0x0683,
    ETH_PROTO_OANSYNC = 0x0684
};

/**
 * @struct LowLatHeader
 * @brief This header contains the layer 2 header of the protocol
 */
struct LowLatHeader {
    uint16_t sender_uid;    /**< Sender device UID */
    uint16_t dest_uid;      /**< Receiver device UID */
    uint16_t psize;         /**< Packet size */
} __attribute__((packed));

/**
 * @struct LowLatPacket
 * @brief Layer 2 packet containing ethernet and addressing headers plus encapsulated data
 * @tparam T Type of the encapsulated data
 */
template<class T>
struct LowLatPacket {
    ethhdr eth_header;  /**< Ethernet header, linux kernel structure */
    LowLatHeader llhdr; /**< LowLatHeader header, custom addressing scheme */
    T payload;          /**< Encapsulated data */
} __attribute__((packed));

/**
 * @struct INT_LLP
 * @brief Custom-sized LowLatPacket with no raw data for encapsulated data. For internal use only
 * @tparam payload_size__ Custom size
 */
template<int payload_size__>
struct INT_LLP : public LowLatPacket<char[payload_size__]> {};

/**
 * @struct IfaceMeta
 * @brief This struct stores physical network interface info.
 */
struct IfaceMeta {
    char mac[6];   /**< MAC Address */
    int idx;       /**< Interface ID (kernel parameter) */
};

#endif // LLSCOMMON_H
