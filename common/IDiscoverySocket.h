// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#ifndef OPENAUDIONETWORK_IDISCOVERY_SOCKET_H
#define OPENAUDIONETWORK_IDISCOVERY_SOCKET_H

#include <cstddef>
#include <cstdint>

#include "packet_structs.h"

/**
 * @class IDiscoverySocket
 * @brief Minimal seam over the discovery EtherType socket.
 *
 * The UID autoconfigurator only needs to broadcast probes/defends and
 * inspect inbound discovery packets. This interface is the smallest
 * surface that lets the algorithm be unit-tested without a real raw
 * socket. The production implementation forwards to LowLatSocket.
 *
 * Receive contract: read one packet's worth of bytes if available
 * within timeout_ms; return 0 on timeout, positive byte count on
 * success, negative on error. The header type discriminator is in the
 * first bytes of the payload (CommonHeader::type after LowLatHeader).
 */
struct IDiscoverySocket {
    virtual ~IDiscoverySocket() = default;

    /** Broadcast a UidProbePacket (dest_uid = 0). Returns bytes sent or <0. */
    virtual int send_probe(const UidProbePacket& pkt) = 0;

    /** Broadcast a UidDefendPacket (dest_uid = 0). Returns bytes sent or <0. */
    virtual int send_defend(const UidDefendPacket& pkt) = 0;

    /**
     * Block up to timeout_ms for the next discovery packet. The buffer
     * receives the full LowLatPacket<T> bytes including ethernet and
     * LowLat headers. The caller dispatches on CommonHeader::type.
     *
     * Returns: byte count on success, 0 on timeout, -1 on error.
     */
    virtual int recv_next(uint8_t* buf, size_t buf_len, int timeout_ms) = 0;
};

#endif // OPENAUDIONETWORK_IDISCOVERY_SOCKET_H
