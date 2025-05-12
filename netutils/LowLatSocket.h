#ifndef LOWLATSOCKET_H
#define LOWLATSOCKET_H

#include <string>
#include <iostream>
#include <cstring>
#include <cassert>
#include <memory>
#include <optional>

#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <sys/ioctl.h>

enum EthProtocol : uint16_t {
    ETH_PROTO_OANAUDIO = 0x0681,
    ETH_PROTO_OANDISCO = 0x0682,
    ETH_PROTO_OANCONTROL = 0x0683
};

struct LowLatHeader {
    uint16_t sender_uid;
    uint16_t dest_uid;
    uint16_t psize;
} __attribute__((packed));

template<class T>
struct LowLatPacket {
    ethhdr eth_header;
    LowLatHeader llhdr;
    T payload;
} __attribute__((packed));

template<int payload_size__>
struct INT_LLP : public LowLatPacket<char[payload_size__]> {};

struct IfaceMeta {
    char mac[6];
    int idx;
};

IfaceMeta get_iface_meta(const std::string& name);

class NetworkMapper;
class LowLatSocket {
public:
    LowLatSocket(uint16_t self_uid, std::shared_ptr<NetworkMapper> mapper);
    ~LowLatSocket();

    bool init_socket(std::string interface, EthProtocol proto);

    template<class T>
    int send_data(const T& data, uint16_t dest_uid) {
        INT_LLP<sizeof(T)> llpck;
        llpck.eth_header = m_hdr;
        llpck.llhdr.dest_uid = dest_uid;
        llpck.llhdr.sender_uid = m_self_uid;
        llpck.llhdr.psize = sizeof(T);
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
            0,
            (sockaddr*)&m_iface_addr,
            sizeof(m_iface_addr)
        );
    }

    template<class T>
    int receive_data(T* data, bool async = true) {
        return recv(m_socket, data, sizeof(T), async ? MSG_DONTWAIT : 0);
    }

private:
    std::optional<uint64_t> get_mac(uint16_t id);

    sockaddr_ll m_iface_addr{};
    ethhdr m_hdr{};

    int m_socket;
    uint16_t m_self_uid;

    std::shared_ptr<NetworkMapper> m_mapper;
};



#endif //LOWLATSOCKET_H
