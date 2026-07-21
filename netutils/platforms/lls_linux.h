// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#ifndef LLSLINUX_H
#define LLSLINUX_H

#if defined(__linux__) && !defined(BUILD_XDP_BACKEND)

#include <string>
#include <cstring>
#include <cassert>
#include <optional>
#include <cstdint>
#include <memory>
#include <iostream>

#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "netutils/lls_common.h"

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
     * Format a given packet assuming it is a LowLatPacket<T> without sending it with the socket interface
     * @param packet_buffer Packet buffer
     * @param dest_uid Packet receiver UID
     * @return true if successfully filled the header
     */
    bool format_packet_header(uint8_t* packet_buffer, uint16_t dest_uid, size_t packet_size);

    /**
     * Write in a given packet assuming it is a LowLatPacket<T>
     * @param packet_buffer Packet buffer
     * @param dest_uid Packet receiver UID
     * @return true if successfully found MAC addr and wrote it to packet
     */
    bool write_packet_mac_addr(uint8_t* packet_buffer, uint16_t dest_uid);

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
        format_packet_header((uint8_t*)&llpck, dest_uid, sizeof(T));
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

        return sendto(
            m_socket,
            &llpck, sizeof(llpck),
            MSG_DONTWAIT,
            (sockaddr*)&m_iface_addr,
            sizeof(m_iface_addr)
        );
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
        return recv(m_socket, data, sizeof(T), async ? MSG_DONTWAIT : 0);
    }

    /**
     * Receive raw data. @see LowLatSocket::receive_data
     * @param data Pointer to the data buffer
     * @param size Amount of data expected
     * @param async This flag set the call as non-blocking if set to true
     * @return Received byte count
     */
    int receive_data_raw(char* data, size_t size, bool async = true) const {
        return recv(m_socket, data, size, async ? MSG_DONTWAIT : 0);
    }

    /**
     * Send raw packet on wiore without further processing
     * @param data Packet data
     * @param size Packet size
     * @return Number of byte sent
     */
    int send_data_raw(char* data, size_t size) {
        return sendto(
            m_socket,
            data, size,
            MSG_DONTWAIT,
            (sockaddr*)&m_iface_addr,
            sizeof(m_iface_addr)
        );
    }

private:
    /**
     * Finds a device MAC address based on its ID.
     * @param id ID to search for
     * @return If found, the corresponding MAC address
     */
    std::optional<uint64_t> get_mac(uint16_t id);

    sockaddr_ll m_iface_addr{};
    ethhdr m_hdr{};

    int m_socket;
    uint16_t m_self_uid;
    EthProtocol m_self_proto;

    std::shared_ptr<NetworkMapper> m_mapper;
};

#endif // __linux__

#endif // LLSLINUX_H
