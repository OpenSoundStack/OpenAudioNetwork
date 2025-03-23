#ifndef LOWLATSOCKET_H
#define LOWLATSOCKET_H

#include <string>
#include <iostream>
#include <cstring>
#include <cassert>

#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define ETH_PROTO_OANAUDIO 0x0681

template<int payload_size__>
struct LowLatPacket {
    ethhdr eth_header;
    uint16_t sender_uid;
    uint16_t dest_uid;
    uint16_t psize;
    char payload[payload_size__];
};

struct IfaceMeta {
    char mac[6];
    int idx;
};

IfaceMeta get_iface_meta(const std::string& name);

class LowLatSocket {
public:
    LowLatSocket(uint16_t self_uid);
    ~LowLatSocket();

    bool init_socket(std::string interface);

    template<class T>
    int send_data(const T& data, uint16_t dest_uid) {
        LowLatPacket<sizeof(T)> llpck;
        llpck.eth_header = m_hdr;
        llpck.dest_uid = dest_uid;
        llpck.sender_uid = m_self_uid;
        llpck.psize = sizeof(T);
        memcpy(llpck.payload, &data, sizeof(T));

        return sendto(
            m_socket,
            &llpck, sizeof(llpck),
            0,
            (sockaddr*)&m_iface_addr,
            sizeof(m_iface_addr)
        );
    }

private:
    sockaddr_ll m_iface_addr{};
    ethhdr m_hdr{};

    int m_socket;
    uint16_t m_self_uid;
};



#endif //LOWLATSOCKET_H
