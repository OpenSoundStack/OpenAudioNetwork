// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#include "LowLatSocket.h"
#include "common/NetworkMapper.h"

#include <iostream>

#ifdef OAN_HOST_BACKENDS
#include "transport/ITransport.h"
#endif

#if !defined(__linux__) && !defined(OAN_HOST_BACKENDS)
// Zephyr-firmware-only hooks. Host builds use ITransport instead.
extern "C" int _send_data(uint8_t* data, size_t data_len);
extern "C" int _recv_data(uint8_t* data_out, size_t data_size, EthProtocol filt_proto);
extern "C" IfaceMeta _fetch_iface_meta(const std::string& name);
#endif

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
    if (m_socket > 0) {
        close(m_socket);
    }
}

bool LowLatSocket::init_socket(std::string interface, EthProtocol proto) {
#ifdef OAN_HOST_BACKENDS
    m_transport = parse_transport(interface);
    if (m_transport) {
        IfaceMeta meta{};
        if (!m_transport->open(interface, proto, m_self_uid, meta)) {
            return false;
        }

        memset(m_hdr.h_dest, 0xFF, 6);
        memcpy(m_hdr.h_source, meta.mac, 6);
        m_hdr.h_proto = htons(proto);
        m_self_proto = proto;
        return true;
    }
#endif

    // Default Linux path: direct AF_PACKET (byte-for-byte unchanged from pre-M3).
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
    m_self_proto = proto;

    return true;
}

IfaceMeta get_iface_meta(const std::string& name) {
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

int LowLatSocket::backend_send(const uint8_t* data, size_t size, uint16_t dest_uid) {
#ifdef OAN_HOST_BACKENDS
    if (m_transport) {
        return m_transport->send(data, size, dest_uid);
    }
#else
    (void)dest_uid;
#endif

    return sendto(
        m_socket,
        data, size,
        MSG_DONTWAIT,
        reinterpret_cast<sockaddr*>(&m_iface_addr),
        sizeof(m_iface_addr)
    );
}

int LowLatSocket::backend_recv(uint8_t* data, size_t size, bool async) const {
#ifdef OAN_HOST_BACKENDS
    if (m_transport) {
        return m_transport->recv(data, size, async);
    }
#endif

    return recv(m_socket, data, size, async ? MSG_DONTWAIT : 0);
}

#elif defined(OAN_HOST_BACKENDS)

LowLatSocket::LowLatSocket(uint16_t self_uid, std::shared_ptr<NetworkMapper> mapper) {
    m_socket = 0;
    memset(m_iface_addr, 0, sizeof(m_iface_addr));
    m_self_uid = self_uid;
    m_mapper = std::move(mapper);
}

LowLatSocket::~LowLatSocket() = default;

bool LowLatSocket::init_socket(std::string interface, EthProtocol proto) {
    m_transport = parse_transport(interface);
    if (!m_transport) {
        std::cerr << "LowLatSocket: this host requires a transport prefix "
                  << "(sim:<name> or raw:<ifname>) or OAN_TRANSPORT env var. "
                  << "Got: '" << interface << "'" << std::endl;
        return false;
    }

    IfaceMeta meta{};
    if (!m_transport->open(interface, proto, m_self_uid, meta)) {
        return false;
    }

    memcpy(m_iface_addr, meta.mac, 6);

    memset(m_hdr.h_dest, 0xFF, 6);
    memcpy(m_hdr.h_source, meta.mac, 6);
    m_hdr.h_proto = htons(proto);
    m_self_proto = proto;

    return true;
}

int LowLatSocket::backend_send(const uint8_t* data, size_t size, uint16_t dest_uid) {
    if (!m_transport) return -1;
    return m_transport->send(data, size, dest_uid);
}

int LowLatSocket::backend_recv(uint8_t* data, size_t size, bool async) const {
    if (!m_transport) return -1;
    return m_transport->recv(data, size, async);
}

#else  // Zephyr firmware path — unchanged from pre-M3

IfaceMeta get_iface_meta(const std::string& name) {
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
    m_self_proto = proto;

    return true;
}

int LowLatSocket::backend_send(const uint8_t* data, size_t size, uint16_t dest_uid) {
    (void)dest_uid;
    // Zephyr's _send_data extern takes non-const uint8_t* for legacy ABI reasons;
    // firmware impls (IO_Board_FW, RackedIO_Board_FW) do not mutate the buffer.
    return send_data_internal(const_cast<uint8_t*>(data), size);
}

int LowLatSocket::backend_recv(uint8_t* data, size_t size, bool async) const {
    (void)async;
    return recv_data_internal(data, size);
}

int LowLatSocket::send_data_internal(uint8_t* data, size_t size) {
    return _send_data(data, size);
}

int LowLatSocket::recv_data_internal(uint8_t* data, size_t size) const {
    return _recv_data(data, size, m_self_proto);
}

#endif
