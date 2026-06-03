#include "ITransport.h"

#include <cstdlib>
#include <cstring>
#include <cassert>
#include <iostream>

#include "../LowLatSocket.h"

#include "SimTransport.h"
#include "RawMacTransport.h"
#ifdef __linux__
  #include "RawLinuxTransport.h"
#endif

#ifdef __linux__
  #include <sys/socket.h>
  #include <sys/ioctl.h>
  #include <net/if.h>
  #include <unistd.h>
#else
  #include <ifaddrs.h>
  #include <net/if.h>
  #include <net/if_dl.h>
#endif

IfaceMeta host_iface_meta(const std::string& iface) {
    IfaceMeta meta{};

#ifdef __linux__
    assert(iface.size() <= 16);

    int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("host_iface_meta: socket()");
        return meta;
    }

    ifreq req{};
    memset(req.ifr_ifrn.ifrn_name, 0x00, 16);
    memcpy(req.ifr_ifrn.ifrn_name, iface.data(), iface.size());

    if (ioctl(sock, SIOCGIFINDEX, &req) < 0) {
        perror("host_iface_meta: SIOCGIFINDEX");
    }
    meta.idx = req.ifr_ifru.ifru_ivalue;

    if (ioctl(sock, SIOCGIFHWADDR, &req) < 0) {
        perror("host_iface_meta: SIOCGIFHWADDR");
    }
    memcpy(meta.mac, req.ifr_ifru.ifru_hwaddr.sa_data, 6);

    ::close(sock);
#else
    ifaddrs* addrs = nullptr;
    if (getifaddrs(&addrs) != 0) {
        perror("host_iface_meta: getifaddrs");
        return meta;
    }

    bool found = false;
    for (ifaddrs* ifa = addrs; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        if (ifa->ifa_addr->sa_family != AF_LINK) continue;
        if (iface != ifa->ifa_name) continue;

        auto* sdl = reinterpret_cast<sockaddr_dl*>(ifa->ifa_addr);
        if (sdl->sdl_alen == 6) {
            memcpy(meta.mac, LLADDR(sdl), 6);
            meta.idx = static_cast<int>(if_nametoindex(iface.c_str()));
            found = true;
            break;
        }
    }

    freeifaddrs(addrs);

    if (!found) {
        std::cerr << "host_iface_meta: no AF_LINK entry for interface '"
                  << iface << "'" << std::endl;
    }
#endif

    return meta;
}

static bool starts_with(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

std::unique_ptr<ITransport> parse_transport(const std::string& iface) {
    if (starts_with(iface, "sim:")) {
        return std::make_unique<SimTransport>(iface.substr(4));
    }
    if (starts_with(iface, "raw:")) {
#ifdef __linux__
        return std::make_unique<RawLinuxTransport>();
#else
        return std::make_unique<RawMacTransport>();
#endif
    }

    const char* env = std::getenv("OAN_TRANSPORT");
    if (env != nullptr) {
        std::string sel{env};
        if (sel == "sim") {
            return std::make_unique<SimTransport>(iface.empty() ? "default" : iface);
        }
        if (sel == "raw") {
#ifdef __linux__
            return std::make_unique<RawLinuxTransport>();
#else
            return std::make_unique<RawMacTransport>();
#endif
        }
    }

    return nullptr;
}
