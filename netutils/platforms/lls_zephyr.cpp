#include "lls_zephyr.h"
#include "common/NetworkMapper.h"

#ifdef __ZEPHYR__

extern "C" int _send_data(uint8_t* data, size_t data_len);
extern "C" int _recv_data(uint8_t* data_out, size_t data_size, EthProtocol filt_proto);

std::optional<uint64_t> LowLatSocket::get_mac(uint16_t id) {
    return m_mapper->get_mac_by_uid(id);
}

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
    m_self_proto = proto;

    return true;
}

int LowLatSocket::send_data_internal(uint8_t *data, size_t size) {
    return _send_data(data, size);
}

int LowLatSocket::recv_data_internal(uint8_t *data, size_t size) const {
    return _recv_data(data, size, m_self_proto);
}

bool LowLatSocket::format_packet_header(uint8_t *packet_buffer, uint16_t dest_uid, size_t packet_size) {
    INT_LLP<1>* llpck = reinterpret_cast<INT_LLP<1> *>(packet_buffer);
    llpck->eth_header = m_hdr;
    llpck->llhdr.dest_uid = dest_uid;
    llpck->llhdr.sender_uid = m_self_uid;
    llpck->llhdr.psize = packet_size;

    if (dest_uid != 0) {
        return write_packet_mac_addr(packet_buffer, dest_uid);
    }

    return true;
}

bool LowLatSocket::write_packet_mac_addr(uint8_t *packet_buffer, uint16_t dest_uid) {
    auto mac = get_mac(dest_uid);

    INT_LLP<1>* llpck = reinterpret_cast<INT_LLP<1> *>(packet_buffer);
    llpck->llhdr.dest_uid = dest_uid;

    if (mac.has_value()) {
        uint64_t& v = mac.value();

        uint64_t* addr = (uint64_t*)llpck->eth_header.h_dest;
        addr[0] = v;

        return true;
    } else {
        return false;
    }
}

#endif // __ZEPHYR__