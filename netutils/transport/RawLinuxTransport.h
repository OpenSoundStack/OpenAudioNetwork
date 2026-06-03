#ifndef OAN_RAW_LINUX_TRANSPORT_H
#define OAN_RAW_LINUX_TRANSPORT_H

#ifdef __linux__

#include <linux/if_packet.h>

#include "ITransport.h"
#include "../LowLatSocket.h"

class RawLinuxTransport : public ITransport {
public:
    RawLinuxTransport() = default;
    ~RawLinuxTransport() override;

    bool open(const std::string& iface, EthProtocol proto,
              uint16_t self_uid, IfaceMeta& out_meta) override;
    int  send(const uint8_t* data, size_t len, uint16_t dest_uid) override;
    int  recv(uint8_t* data, size_t len, bool async) override;
    void close() override;

private:
    int          m_socket{-1};
    sockaddr_ll  m_iface_addr{};
};

#endif // __linux__

#endif
