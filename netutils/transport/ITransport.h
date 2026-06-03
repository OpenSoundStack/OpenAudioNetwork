#ifndef OAN_ITRANSPORT_H
#define OAN_ITRANSPORT_H

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>

struct IfaceMeta;
enum EthProtocol : uint16_t;

struct ITransport {
    virtual ~ITransport() = default;

    virtual bool open(const std::string& iface, EthProtocol proto,
                      uint16_t self_uid, IfaceMeta& out_meta) = 0;
    virtual int  send(const uint8_t* data, size_t len, uint16_t dest_uid) = 0;
    virtual int  recv(uint8_t* data, size_t len, bool async) = 0;
    virtual void close() = 0;
};

IfaceMeta host_iface_meta(const std::string& iface);

std::unique_ptr<ITransport> parse_transport(const std::string& iface);

#endif
