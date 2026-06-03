#ifndef OAN_SIM_TRANSPORT_H
#define OAN_SIM_TRANSPORT_H

#include <cstdint>
#include <string>
#include <vector>

#include "ITransport.h"
#include "../LowLatSocket.h"  // EthProtocol

class SimTransport : public ITransport {
public:
    // Argument is the post-"sim:" string from parse_transport — either
    // "<daemon-name>" or "<daemon-name>,mac=02:00:00:00:00:01".
    // Empty string defaults to daemon name "default".
    explicit SimTransport(std::string daemon_name_with_opts);
    ~SimTransport() override;

    bool open(const std::string& iface, EthProtocol proto,
              uint16_t self_uid, IfaceMeta& out_meta) override;
    int  send(const uint8_t* data, size_t len, uint16_t dest_uid) override;
    int  recv(uint8_t* data, size_t len, bool async) override;
    void close() override;

private:
    void parse_opts(const std::string& s);

    std::string m_daemon_name;
    std::string m_socket_path;
    uint8_t     m_self_mac[6]{};
    bool        m_mac_pinned{false};

    int         m_fd{-1};
    EthProtocol m_proto{};
    uint16_t    m_self_uid{0};

    bool                 m_have_hdr{false};
    uint8_t              m_hdr_buf[8]{};       // sizeof(SimFrame) == 8
    size_t               m_hdr_read{0};
    std::vector<uint8_t> m_body_buf;
    size_t               m_body_read{0};
    uint32_t             m_body_expected{0};
};

#endif
