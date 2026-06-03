#include "SimTransport.h"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <utility>

#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <unistd.h>

// NOTE: wire framing is owned by the OALS sim_switch dev tool (which is the
// only program SimTransport can talk to). We deliberately do NOT include a
// header from OALS — OAN is vendored into firmware repos and must stay
// self-contained. The structs below are a local copy that must stay
// byte-identical with tools/sim_switch/sim_proto.h. The framing magic and
// version are the contract check.

namespace {

constexpr uint32_t SIM_MAGIC          = 0x4F535354;  // 'OSST'
constexpr uint8_t  SIM_VERSION        = 1;
constexpr uint32_t MAX_FRAME_PAYLOAD  = 8192;        // matches sim_switch

struct SimHello {
    uint32_t magic;
    uint8_t  version;
    uint8_t  _pad;
    uint16_t ethertype;
    uint16_t self_uid;
    uint16_t _reserved;
} __attribute__((packed));

struct SimFrame {
    uint32_t payload_len;
    uint16_t ethertype;
    uint16_t dest_uid;
} __attribute__((packed));

bool parse_mac(const std::string& s, uint8_t out[6]) {
    unsigned int v[6];
    if (std::sscanf(s.c_str(),
                    "%x:%x:%x:%x:%x:%x",
                    &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) != 6) {
        return false;
    }
    for (int i = 0; i < 6; ++i) {
        if (v[i] > 0xFF) return false;
        out[i] = static_cast<uint8_t>(v[i]);
    }
    return true;
}

void random_locally_admin_mac(uint8_t out[6]) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> d(0, 255);
    out[0] = 0x02;  // locally administered, unicast
    for (int i = 1; i < 6; ++i) out[i] = static_cast<uint8_t>(d(gen));
}

// One MAC per process. The engine constructs 4 SimTransports (audio, disco,
// control, sync) — they must all carry the same MAC so peers see a single
// device identity. OAN_SIM_MAC env var can override; otherwise a random
// locally-administered MAC is minted once and reused across all instances.
const uint8_t* process_self_mac() {
    static uint8_t s_mac[6];
    static bool    s_init = false;
    if (s_init) return s_mac;

    const char* env = std::getenv("OAN_SIM_MAC");
    if (env) {
        if (parse_mac(env, s_mac)) {
            s_init = true;
            return s_mac;
        }
        std::cerr << "SimTransport: ignoring malformed OAN_SIM_MAC='"
                  << env << "' (expected 02:00:00:00:00:01)\n";
    }
    random_locally_admin_mac(s_mac);
    s_init = true;
    return s_mac;
}

} // namespace

SimTransport::SimTransport(std::string s) {
    parse_opts(s);
    m_socket_path = "/tmp/osst-sim-" + m_daemon_name + ".sock";
    if (!m_mac_pinned) std::memcpy(m_self_mac, process_self_mac(), 6);
}

SimTransport::~SimTransport() {
    close();
}

void SimTransport::parse_opts(const std::string& s) {
    // Split on commas: first token = daemon name, subsequent tokens = "key=value".
    size_t start = 0;
    int tok_idx = 0;
    while (start <= s.size()) {
        size_t comma = s.find(',', start);
        std::string tok = s.substr(start, comma == std::string::npos ? std::string::npos : comma - start);

        if (tok_idx == 0) {
            m_daemon_name = tok.empty() ? "default" : tok;
        } else {
            auto eq = tok.find('=');
            if (eq != std::string::npos) {
                std::string key = tok.substr(0, eq);
                std::string val = tok.substr(eq + 1);
                if (key == "mac") {
                    if (parse_mac(val, m_self_mac)) {
                        m_mac_pinned = true;
                    } else {
                        std::cerr << "SimTransport: ignoring malformed mac='"
                                  << val << "' (expected 02:00:00:00:00:01)\n";
                    }
                } else {
                    std::cerr << "SimTransport: ignoring unknown option '"
                              << key << "'\n";
                }
            }
        }

        if (comma == std::string::npos) break;
        start = comma + 1;
        tok_idx++;
    }

    if (m_daemon_name.empty()) m_daemon_name = "default";
}

bool SimTransport::open(const std::string& iface, EthProtocol proto,
                        uint16_t self_uid, IfaceMeta& out_meta) {
    (void)iface;  // already parsed in ctor

    m_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_fd < 0) {
        std::cerr << "SimTransport: socket() failed: " << ::strerror(errno) << "\n";
        return false;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    if (m_socket_path.size() >= sizeof(addr.sun_path)) {
        std::cerr << "SimTransport: socket path too long\n";
        ::close(m_fd); m_fd = -1;
        return false;
    }
    std::strncpy(addr.sun_path, m_socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (::connect(m_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "SimTransport: connect to " << m_socket_path
                  << " failed (is sim_switch running?): "
                  << ::strerror(errno) << "\n";
        ::close(m_fd); m_fd = -1;
        return false;
    }

    SimHello h{
        SIM_MAGIC,
        SIM_VERSION,
        0,
        static_cast<uint16_t>(proto),
        self_uid,
        0
    };
    ssize_t hn = ::send(m_fd, &h, sizeof(h), 0);
    if (hn != static_cast<ssize_t>(sizeof(h))) {
        std::cerr << "SimTransport: hello send failed: " << ::strerror(errno) << "\n";
        ::close(m_fd); m_fd = -1;
        return false;
    }

    int flags = ::fcntl(m_fd, F_GETFL, 0);
    if (flags < 0 || ::fcntl(m_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cerr << "SimTransport: fcntl(O_NONBLOCK) failed: "
                  << ::strerror(errno) << "\n";
        ::close(m_fd); m_fd = -1;
        return false;
    }

    // SIGPIPE protection: macOS has no MSG_NOSIGNAL, so if the daemon dies
    // mid-stream our next sendmsg would deliver SIGPIPE and kill the engine.
    // Ignoring it process-wide is a library-side side effect, but it's the
    // only portable knob — engines that need EPIPE behaviour explicitly
    // were going to die from SIGPIPE anyway. Idempotent + harmless on Linux.
    static bool s_sigpipe_ignored = false;
    if (!s_sigpipe_ignored) {
        std::signal(SIGPIPE, SIG_IGN);
        s_sigpipe_ignored = true;
    }

    // Pre-reserve the recv body buffer to the largest legal frame size so the
    // audio hot path (recv at ~750 Hz/channel) never triggers a reallocation.
    m_body_buf.reserve(MAX_FRAME_PAYLOAD);

    m_proto    = proto;
    m_self_uid = self_uid;
    std::memcpy(out_meta.mac, m_self_mac, 6);
    out_meta.idx = 0;
    return true;
}

int SimTransport::send(const uint8_t* data, size_t len, uint16_t dest_uid) {
    if (m_fd < 0) return -1;

    SimFrame hdr{
        static_cast<uint32_t>(len),
        static_cast<uint16_t>(m_proto),
        dest_uid
    };

    iovec iov[2] = {
        { &hdr, sizeof(hdr) },
        { const_cast<uint8_t*>(data), len }
    };
    msghdr m{};
    m.msg_iov    = iov;
    m.msg_iovlen = 2;

    ssize_t n;
    do {
        n = ::sendmsg(m_fd, &m, MSG_DONTWAIT
#ifdef __linux__
                      | MSG_NOSIGNAL
#endif
                      );
    } while (n < 0 && errno == EINTR);

    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
        return -1;
    }
    return static_cast<int>(len);  // caller cares about payload bytes
}

int SimTransport::recv(uint8_t* data, size_t len, bool async) {
    if (m_fd < 0) return -1;

    while (true) {
        // Phase 1: read SimFrame header.
        if (!m_have_hdr) {
            while (m_hdr_read < sizeof(m_hdr_buf)) {
                ssize_t n = ::read(m_fd, m_hdr_buf + m_hdr_read,
                                   sizeof(m_hdr_buf) - m_hdr_read);
                if (n > 0) { m_hdr_read += n; continue; }
                if (n == 0) return -1;  // peer closed
                if (errno == EINTR) continue;
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    if (async) return 0;
                    // Blocking mode: wait for more data.
                    pollfd p{m_fd, POLLIN, 0};
                    int pr = ::poll(&p, 1, -1);
                    if (pr < 0 && errno != EINTR) return -1;
                    continue;
                }
                return -1;
            }
            SimFrame hdr{};
            std::memcpy(&hdr, m_hdr_buf, sizeof(hdr));
            if (hdr.payload_len > MAX_FRAME_PAYLOAD) {
                std::cerr << "SimTransport: oversize frame len="
                          << hdr.payload_len << "; disconnecting\n";
                return -1;
            }
            m_body_expected = hdr.payload_len;
            // resize() value-initialises, but we always overwrite via read()
            // before reading m_body_buf[i], so the zero-fill is wasted work.
            // Since reserve(MAX_FRAME_PAYLOAD) ran in open(), no allocation
            // happens here — only setting size().
            m_body_buf.resize(m_body_expected);
            m_body_read = 0;
            m_have_hdr = true;
        }

        // Phase 2: read body.
        while (m_body_read < m_body_expected) {
            ssize_t n = ::read(m_fd, m_body_buf.data() + m_body_read,
                               m_body_expected - m_body_read);
            if (n > 0) { m_body_read += n; continue; }
            if (n == 0) return -1;
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (async) return 0;
                pollfd p{m_fd, POLLIN, 0};
                int pr = ::poll(&p, 1, -1);
                if (pr < 0 && errno != EINTR) return -1;
                continue;
            }
            return -1;
        }

        // Phase 3: deliver.
        size_t out_len = std::min<size_t>(len, m_body_expected);
        if (m_body_expected > len) {
            // Caller's buffer was smaller than the payload. This matches
            // AF_PACKET's truncation semantics but is almost certainly a sizing
            // bug — log once per frame to aid debugging without spamming.
            std::cerr << "SimTransport: recv truncated frame ("
                      << m_body_expected << " → " << len << " bytes)\n";
        }
        std::memcpy(data, m_body_buf.data(), out_len);

        // Reset state for next frame. Keep m_body_buf capacity intact
        // (resize(0) preserves capacity; clear() does too, but resize is the
        // explicit "shrink length, retain storage" idiom).
        m_have_hdr = false;
        m_hdr_read = 0;
        m_body_read = 0;
        m_body_expected = 0;
        m_body_buf.resize(0);

        return static_cast<int>(out_len);
    }
}

void SimTransport::close() {
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
}
