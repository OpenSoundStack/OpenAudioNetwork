#ifndef OAN_RAW_MAC_TRANSPORT_H
#define OAN_RAW_MAC_TRANSPORT_H

#include "ITransport.h"

class RawMacTransport : public ITransport {
public:
    RawMacTransport() = default;
    ~RawMacTransport() override = default;

    bool open(const std::string& iface, EthProtocol proto,
              uint16_t self_uid, IfaceMeta& out_meta) override;
    int  send(const uint8_t* data, size_t len, uint16_t dest_uid) override;
    int  recv(uint8_t* data, size_t len, bool async) override;
    void close() override;
};

#endif
