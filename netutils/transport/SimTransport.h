#ifndef OAN_SIM_TRANSPORT_H
#define OAN_SIM_TRANSPORT_H

#include "ITransport.h"

class SimTransport : public ITransport {
public:
    explicit SimTransport(std::string daemon_name);
    ~SimTransport() override;

    bool open(const std::string& iface, EthProtocol proto,
              uint16_t self_uid, IfaceMeta& out_meta) override;
    int  send(const uint8_t* data, size_t len, uint16_t dest_uid) override;
    int  recv(uint8_t* data, size_t len, bool async) override;
    void close() override;

private:
    std::string m_daemon_name;
};

#endif
