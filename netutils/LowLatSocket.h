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

#ifndef LOWLATSOCKET_H
#define LOWLATSOCKET_H

#ifdef __linux__
#include <iostream>
#endif //__linux__

#include <string>
#include <cstring>
#include <cassert>
#include <optional>
#include <cstdint>
#include <memory>

#ifdef __linux__
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <sys/ioctl.h>
#else

// Defining linux-like structs for compat
struct EthernetHeader {
    uint8_t h_dest[6];
    uint8_t h_source[6];
    uint16_t h_proto;
} __attribute__((packed));
typedef EthernetHeader ethhdr;

#define htons(x) ((((x) & 0xFF00) >> 8) | (((x) & 0x00FF) << 8))

#endif //__linux__

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

/**
 * Function to retreive a given network interface infos.
 * @param name Interface name
 * @return Interface infos if found. All zeros if not found.
 */
IfaceMeta get_iface_meta(const std::string& name);

class NetworkMapper;

/**
 * @class LowLatSocket
 * @brief Wrapper for raw sockets that implements a layer 2 addressing protocol.
 */
class LowLatSocket {
public:
    /**
     * Constructor
     * @param self_uid Host UID
     * @param mapper Local OAN network mapper
     */
    LowLatSocket(uint16_t self_uid, std::shared_ptr<NetworkMapper> mapper);
    ~LowLatSocket();

    /**
     * Initializes the socket
     * @param interface Physical network interface name to attach the socket to
     * @param proto Ethernet protocol used. @see EthProtocol
     * @return true if initialization succeeds
     */
    bool init_socket(std::string interface, EthProtocol proto);

    /**
     * Sends some data on the network
     * @tparam T Sent data type
     * @param data Pointer to the data
     * @param dest_uid Package receiver UID
     * @return Sent byte count. Less than 0 if error.
     */
    template<class T>
    int send_data(const T& data, uint16_t dest_uid) {
        INT_LLP<sizeof(T)> llpck;
        llpck.eth_header = m_hdr;
        llpck.llhdr.dest_uid = dest_uid;
        llpck.llhdr.sender_uid = m_self_uid;
        llpck.llhdr.psize = sizeof(T);
        memcpy(llpck.payload, &data, sizeof(T));

        if (dest_uid != 0) {
            auto mac = get_mac(dest_uid);
            if (mac.has_value()) {
                uint64_t& v = mac.value();
                memcpy(llpck.eth_header.h_dest, &v, 6);
            } else {
                //std::cerr << "Trying to send data to unknown UID (" << (int)dest_uid << ")." << std::endl;
                return 0;
            }
        }

#ifdef __linux__
        return sendto(
            m_socket,
            &llpck, sizeof(llpck),
            MSG_DONTWAIT,
            (sockaddr*)&m_iface_addr,
            sizeof(m_iface_addr)
        );
#else
        return send_data_internal((uint8_t*)&llpck, sizeof(T));
#endif // __linux__
    }

    /**
     * Receive some data
     * @tparam T Data type received
     * @param data Pointer to the data buffer
     * @param async This flag set the call as non-blocking if set to true
     * @return Received byte count
     */
    template<class T>
    int receive_data(T* data, bool async = true) {
#ifdef __linux__
        return recv(m_socket, data, sizeof(T), async ? MSG_DONTWAIT : 0);
#else
        return recv_data_internal((uint8_t*)data, sizeof(T));
#endif // __linux__
    }

    /**
     * Receive raw data. @see LowLatSocket::receive_data
     * @param data Pointer to the data buffer
     * @param size Amount of data expected
     * @param async This flag set the call as non-blocking if set to true
     * @return Received byte count
     */
    int receive_data_raw(char* data, size_t size, bool async = true) const {
#ifdef __linux__
        return recv(m_socket, data, size, async ? MSG_DONTWAIT : 0);
#else
        return recv_data_internal((uint8_t*)data, size);
#endif // __linux__
    }

private:
    /**
     * Finds a device MAC address based on its ID.
     * @param id ID to search for
     * @return If found, the corresponding MAC address
     */
    std::optional<uint64_t> get_mac(uint16_t id);

#ifndef __linux__
    int send_data_internal(uint8_t* data, size_t size);
    int recv_data_internal(uint8_t* data, size_t size) const;
#endif // __linux__

#ifdef __linux__
    sockaddr_ll m_iface_addr{};
#else
    uint8_t m_iface_addr[6];
#endif // __linux__
    ethhdr m_hdr{};

    int m_socket;
    uint16_t m_self_uid;

    std::shared_ptr<NetworkMapper> m_mapper;
};



#endif //LOWLATSOCKET_H
