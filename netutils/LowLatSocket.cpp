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

#include "LowLatSocket.h"
#include "common/NetworkMapper.h"

#ifndef __linux__
extern "C" int _send_data(uint8_t* data, size_t data_len);
extern "C" int _recv_data(uint8_t* data_out, size_t data_size);
#endif // __linux__

std::optional<uint64_t> LowLatSocket::get_mac(uint16_t id) {
    return m_mapper->get_mac_by_uid(id);
}

#ifdef __linux__

LowLatSocket::LowLatSocket(uint16_t self_uid, std::shared_ptr<NetworkMapper> mapper) {
    m_socket = 0;
    m_iface_addr = {};
    m_self_uid = self_uid;
    m_mapper = std::move(mapper);
}

LowLatSocket::~LowLatSocket() {
    close(m_socket);
}

bool LowLatSocket::init_socket(std::string interface, EthProtocol proto) {
    m_socket = socket(AF_PACKET, SOCK_RAW, htons(proto));
    if (m_socket < 0) {
        std::cerr << "LLS Failed to open low level socket. Err = " << errno << std::endl;
        return false;
    }

    IfaceMeta meta = get_iface_meta(interface);

    m_iface_addr.sll_ifindex = meta.idx;
    m_iface_addr.sll_halen = ETH_ALEN;
    m_iface_addr.sll_protocol = htons(proto);
    memcpy(m_iface_addr.sll_addr, meta.mac, 6);

    memset(m_hdr.h_dest, 0xFF, 6);
    memcpy(m_hdr.h_source, meta.mac, 6);
    m_hdr.h_proto = htons(proto);

    return true;
}


IfaceMeta get_iface_meta(const std::string &name) {
    // Overflow check
    assert(name.size() <= 16);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Failed to open temporary socket");
    }

    ifreq req{};
    memset(req.ifr_ifrn.ifrn_name, 0x00, 16);
    memcpy(req.ifr_ifrn.ifrn_name, name.data(), name.size());

    IfaceMeta meta{};

    if (ioctl(sock, SIOCGIFINDEX, &req) < 0) {
        perror("LLS Failed to get iface index");
    }
    meta.idx = req.ifr_ifru.ifru_ivalue;

    if (ioctl(sock, SIOCGIFHWADDR, &req) < 0) {
        perror("LLS Failed to get iface MAC address");
    }
    memcpy(meta.mac, req.ifr_ifru.ifru_hwaddr.sa_data, 6);

    close(sock);

    return meta;
}

#else
// Embeddable interface that must be private
extern "C" IfaceMeta _fetch_iface_meta(std::string& name);

IfaceMeta get_iface_meta(std::string& name) {
    return _fetch_iface_meta(name);
}

LowLatSocket::LowLatSocket(uint16_t self_uid, std::shared_ptr<NetworkMapper> mapper) {
    m_socket = 0;
    m_self_uid = self_uid;
    m_mapper = std::move(mapper);
}

LowLatSocket::~LowLatSocket() {

}

bool LowLatSocket::init_socket(std::string interface, EthProtocol proto) {
    IfaceMeta meta = get_iface_meta(interface);
    memcpy(m_iface_addr, meta.mac, 6);

    memset(m_hdr.h_dest, 0xFF, 6);
    memcpy(m_hdr.h_source, meta.mac, 6);
    m_hdr.h_proto = htons(proto);

    return true;
}

int LowLatSocket::send_data_internal(uint8_t *data, size_t size) {
    return _send_data(data, size);
}

int LowLatSocket::recv_data_internal(uint8_t *data, size_t size) const {
    return _recv_data(data, size);
}

#endif // __linux__
