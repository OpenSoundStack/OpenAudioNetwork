#include "SimTransport.h"

#include <iostream>
#include <utility>

SimTransport::SimTransport(std::string daemon_name)
    : m_daemon_name(std::move(daemon_name)) {}

SimTransport::~SimTransport() = default;

bool SimTransport::open(const std::string& iface, EthProtocol proto,
                        uint16_t self_uid, IfaceMeta& out_meta) {
    (void)iface; (void)proto; (void)self_uid; (void)out_meta;
    std::cerr << "SimTransport: not yet implemented (M4) — daemon name '"
              << m_daemon_name << "'" << std::endl;
    return false;
}

int SimTransport::send(const uint8_t* data, size_t len, uint16_t dest_uid) {
    (void)data; (void)len; (void)dest_uid;
    return -1;
}

int SimTransport::recv(uint8_t* data, size_t len, bool async) {
    (void)data; (void)len; (void)async;
    return -1;
}

void SimTransport::close() {}
