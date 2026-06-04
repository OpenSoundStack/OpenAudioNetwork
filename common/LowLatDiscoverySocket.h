// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#ifndef OPENAUDIONETWORK_LOWLAT_DISCOVERY_SOCKET_H
#define OPENAUDIONETWORK_LOWLAT_DISCOVERY_SOCKET_H

#include "IDiscoverySocket.h"
#include "netutils/LowLatSocket.h"

/**
 * @class LowLatDiscoverySocket
 * @brief Production IDiscoverySocket: thin adapter over LowLatSocket.
 *
 * Used by the boot-time autoconfigurator. After commit the same
 * LowLatSocket is reused by NetworkMapper for normal mapping traffic.
 */
class LowLatDiscoverySocket : public IDiscoverySocket {
public:
    explicit LowLatDiscoverySocket(LowLatSocket& sock) : m_sock(sock) {}

    int send_probe(const UidProbePacket& pkt) override {
        return m_sock.send_data(pkt, /*dest_uid=*/0);
    }

    int send_defend(const UidDefendPacket& pkt) override {
        return m_sock.send_data(pkt, /*dest_uid=*/0);
    }

    int recv_next(uint8_t* buf, size_t buf_len, int timeout_ms) override {
        int r = m_sock.wait_readable(timeout_ms);
        if (r <= 0) return r; // 0 timeout, -1 error
        return m_sock.receive_data_raw(reinterpret_cast<char*>(buf), buf_len,
                                       /*async=*/true);
    }

private:
    LowLatSocket& m_sock;
};

#endif // OPENAUDIONETWORK_LOWLAT_DISCOVERY_SOCKET_H
