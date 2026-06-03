#include "RawLinuxTransport.h"

#ifdef __linux__

#include <iostream>
#include <cstring>
#include <cerrno>

#include <sys/socket.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <unistd.h>

RawLinuxTransport::~RawLinuxTransport() {
    close();
}

bool RawLinuxTransport::open(const std::string& iface, EthProtocol proto,
                             uint16_t self_uid, IfaceMeta& out_meta) {
    (void)self_uid;

    m_socket = ::socket(AF_PACKET, SOCK_RAW, htons(proto));
    if (m_socket < 0) {
        std::cerr << "RawLinuxTransport: socket() failed, errno=" << errno << std::endl;
        return false;
    }

    out_meta = host_iface_meta(iface);

    m_iface_addr.sll_ifindex  = out_meta.idx;
    m_iface_addr.sll_halen    = ETH_ALEN;
    m_iface_addr.sll_protocol = htons(proto);
    memcpy(m_iface_addr.sll_addr, out_meta.mac, 6);

    return true;
}

int RawLinuxTransport::send(const uint8_t* data, size_t len, uint16_t dest_uid) {
    (void)dest_uid;
    return ::sendto(
        m_socket,
        data, len,
        MSG_DONTWAIT,
        reinterpret_cast<const sockaddr*>(&m_iface_addr),
        sizeof(m_iface_addr)
    );
}

int RawLinuxTransport::recv(uint8_t* data, size_t len, bool async) {
    return ::recv(m_socket, data, len, async ? MSG_DONTWAIT : 0);
}

void RawLinuxTransport::close() {
    if (m_socket >= 0) {
        ::close(m_socket);
        m_socket = -1;
    }
}

#endif // __linux__
