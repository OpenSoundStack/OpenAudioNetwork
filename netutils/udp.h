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

#ifndef OSST_NET_UDP_H
#define OSST_NET_UDP_H

#include <iostream>
#include <cstdint>
#include <memory>

#ifdef __linux__
#include <unistd.h>
#include <netinet/udp.h>
#include <linux/net_tstamp.h>
#include <arpa/inet.h>
#include <fcntl.h>

constexpr uint32_t ip(const uint32_t a, const uint32_t b, const uint32_t c, uint32_t d) {
    return (d << 24) | ((c << 16) & 0xFF0000) | ((b << 8) & 0xFF00) | (a);
}

class UDPSocket {
public:
    UDPSocket();
    ~UDPSocket();

    bool init_socket(uint32_t bound_ip, uint16_t port);
    void enable_broadcasting() const;
    void set_high_prio() const;

    template<class T>
    int receive_data(T* buffer, const size_t buffer_size,  sockaddr_in& client, bool async = true) {
        constexpr int sz = sizeof(sockaddr_in);
        return recvfrom(m_socket, buffer, buffer_size * sizeof(T), async ? MSG_DONTWAIT : 0, (sockaddr*)&client, (socklen_t*)&sz);
    }

    template<class T>
    int send_data(T* buffer, const size_t buffer_size, const uint32_t ip, const uint16_t port) {
        sockaddr_in receiver{};
        receiver.sin_family = AF_INET;
        receiver.sin_port = htons(port);
        receiver.sin_addr.s_addr = ip;

        return sendto(m_socket, buffer, buffer_size * sizeof(T), 0, (sockaddr*)&receiver, sizeof(sockaddr_in));
    }

    uint32_t get_ip() const;
    uint16_t get_port() const;
private:
    int m_socket;
    sockaddr_in m_self;
};
#endif // __linux__
#endif //OSST_NET_UDP_H
