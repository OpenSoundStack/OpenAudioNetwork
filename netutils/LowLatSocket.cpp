#include "LowLatSocket.h"

LowLatSocket::LowLatSocket(uint16_t self_uid) {
    m_socket = 0;
    m_iface_addr = {};

    m_self_uid = self_uid;
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

