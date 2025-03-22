#include "udp.h"

UDPSocket::UDPSocket() {
}

UDPSocket::~UDPSocket() {
    close(m_socket);
}

bool UDPSocket::init_socket(const uint32_t bound_ip, const uint16_t port) {
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(m_socket < 0) {
        return false;
    }

    enable_broadcasting();

    m_self.sin_family = AF_INET;
    m_self.sin_addr.s_addr = htonl(bound_ip);
    m_self.sin_port = htons(port);

    int bind_res = bind(m_socket, (sockaddr*)&m_self, sizeof(m_self));
    if(bind_res < 0) {
        std::cout << "Error no: " << errno << std::endl;
        return false;
    }

    return true;
}

uint32_t UDPSocket::get_ip() const {
    return m_self.sin_addr.s_addr;
}

uint16_t UDPSocket::get_port() const {
    return m_self.sin_port;
}

void UDPSocket::enable_broadcasting() const {
    int br_en = 1;
    setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST, &br_en, sizeof(int));
    setsockopt(m_socket, SOL_SOCKET, SO_REUSEPORT, &br_en, sizeof(int));
}

void UDPSocket::set_high_prio() const {
    int prio = 6;
    setsockopt(m_socket, SOL_SOCKET, SO_PRIORITY, &prio, sizeof(int));

    int enable = SOF_TIMESTAMPING_TX_HARDWARE | SOF_TIMESTAMPING_TX_SOFTWARE;
    setsockopt(m_socket, SOL_SOCKET, SO_TIMESTAMPING, &enable, sizeof(enable));
}

