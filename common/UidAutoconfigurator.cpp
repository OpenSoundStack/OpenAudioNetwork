// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#include "UidAutoconfigurator.h"

#include <cstring>
#include <iostream>

#include "netutils/LowLatSocket.h" // LowLatPacket layout for dispatch

namespace {

// FNV-1a 32-bit over MAC || salt, folded to 16 bits, clamped to dynamic.
uint32_t fnv1a32(const uint8_t* data, size_t len) {
    constexpr uint32_t FNV_OFFSET = 0x811C9DC5u;
    constexpr uint32_t FNV_PRIME  = 0x01000193u;
    uint32_t h = FNV_OFFSET;
    for (size_t i = 0; i < len; ++i) {
        h ^= data[i];
        h *= FNV_PRIME;
    }
    return h;
}

uint64_t mac_to_u64(const uint8_t mac[6]) {
    uint64_t v = 0;
    for (int i = 0; i < 6; ++i) {
        v = (v << 8) | mac[i];
    }
    return v;
}

bool macs_equal(const uint8_t a[6], uint64_t b_low48) {
    uint64_t a_u = 0;
    for (int i = 0; i < 6; ++i) a_u = (a_u << 8) | a[i];
    return a_u == (b_low48 & 0x0000FFFFFFFFFFFFull);
}

} // namespace

uint16_t UidAutoconfigurator::hash16(const uint8_t mac[6], uint8_t salt) {
    uint8_t buf[7];
    std::memcpy(buf, mac, 6);
    buf[6] = salt;
    uint32_t h32 = fnv1a32(buf, sizeof(buf));
    uint16_t folded = static_cast<uint16_t>((h32 >> 16) ^ (h32 & 0xFFFFu));
    // Clamp into dynamic range. Map (0) and (>UID_DYNAMIC_HI) into the
    // valid window without biasing the distribution noticeably.
    if (folded < UID_DYNAMIC_LO) folded = UID_DYNAMIC_LO;
    if (folded > UID_DYNAMIC_HI) {
        // Wrap via modulo over the dynamic-range width. UID_DYNAMIC_HI -
        // UID_DYNAMIC_LO + 1 = 0xEFFF.
        constexpr uint32_t span = UID_DYNAMIC_HI - UID_DYNAMIC_LO + 1;
        folded = static_cast<uint16_t>(UID_DYNAMIC_LO + (folded % span));
    }
    return folded;
}

UidAutoconfigurator::UidAutoconfigurator(const uint8_t self_mac[6],
                                         uint16_t hint_uid,
                                         IUidStore& store,
                                         IDiscoverySocket& sock,
                                         IUidClock& clock)
    : m_hint(hint_uid), m_store(store), m_sock(sock), m_clock(clock) {
    std::memcpy(m_mac, self_mac, 6);
    m_mac_u64 = mac_to_u64(self_mac);
}

UidAutoconfigResult UidAutoconfigurator::run() {
    UidAutoconfigResult res{};

    // 1. Honour static-range hint if present.
    if (m_hint != 0) {
        if (is_static(m_hint)) {
            res.committed_uid = m_hint;
            res.salt_used = 0;
            res.used_persisted = false;
            res.had_collision = false;
            // Static-range UIDs bypass the probe phase per design §2.5.
            // Runtime defence still applies, but it lives in NetworkMapper.
            return res;
        }
        if (is_dynamic(m_hint)) {
            std::cerr << "UidAutoconfigurator: dynamic-range hint UID 0x"
                      << std::hex << m_hint << std::dec
                      << " ignored; autoconfiguration will choose a UID." << std::endl;
        } else {
            std::cerr << "UidAutoconfigurator: invalid hint UID 0x"
                      << std::hex << m_hint << std::dec << " ignored." << std::endl;
        }
    }

    // 2. Try persisted UID first.
    auto persisted = m_store.load();
    uint16_t candidate;
    uint8_t  salt;
    bool     trying_persisted;
    if (persisted && is_dynamic(*persisted)) {
        candidate = *persisted;
        salt = 0; // unknown, salt not persisted by design
        trying_persisted = true;
    } else {
        if (persisted && !is_dynamic(*persisted)) {
            std::cerr << "UidAutoconfigurator: persisted UID 0x"
                      << std::hex << *persisted << std::dec
                      << " is out of dynamic range; ignoring." << std::endl;
            m_store.clear();
        }
        salt = 0;
        candidate = hash16(m_mac, salt);
        trying_persisted = false;
    }

    // 3. ALOHA jitter (once, before the first probe).
    if (m_tunables.jitter_max_us > 0) {
        uint32_t r = m_clock.random_u32();
        uint64_t jitter = r % (m_tunables.jitter_max_us + 1);
        m_clock.sleep_us(jitter);
    }

    // 4. Probe-and-walk loop.
    for (uint8_t attempt = 0; attempt <= m_tunables.max_salt; ++attempt) {
        bool conflict = false;
        if (!probe_candidate(candidate, salt, conflict)) {
            std::cerr << "UidAutoconfigurator: probe send failed; committing "
                         "without verification." << std::endl;
            // Fall through to commit — better than refusing to boot. Live
            // collision detection in NetworkMapper will catch it later.
            conflict = false;
        }

        if (!conflict) {
            res.committed_uid = candidate;
            res.salt_used = salt;
            res.used_persisted = trying_persisted;
            // had_collision is set true only if a non-final attempt collided.
            // (attempt == 0 with no conflict means clean commit.)
            // res.had_collision already set by earlier iterations below.
            m_store.save(candidate);
            return res;
        }

        // Conflict path.
        res.had_collision = true;
        if (trying_persisted) {
            // Persisted UID was contested — lose it and restart from salt 0.
            std::cerr << "UidAutoconfigurator: persisted UID 0x"
                      << std::hex << candidate << std::dec
                      << " contested on boot; restarting salt walk." << std::endl;
            m_store.clear();
            trying_persisted = false;
            salt = 0;
            candidate = hash16(m_mac, salt);
        } else {
            salt = static_cast<uint8_t>(salt + 1);
            candidate = hash16(m_mac, salt);
        }
    }

    // Exhausted max_salt without finding a free UID. Pathological — segment
    // is impossibly crowded or fake socket misbehaving. Commit anyway with
    // the last candidate; the operator-notification layer (TBD) is the
    // right place to scream about this.
    std::cerr << "UidAutoconfigurator: exhausted salt walk after "
              << (int)m_tunables.max_salt << " attempts. Committing 0x"
              << std::hex << candidate << std::dec
              << " — segment may be in serious trouble." << std::endl;
    res.committed_uid = candidate;
    res.salt_used = salt;
    res.used_persisted = false;
    m_store.save(candidate);
    return res;
}

bool UidAutoconfigurator::probe_candidate(uint16_t candidate,
                                          uint8_t salt,
                                          bool& out_conflict) {
    out_conflict = false;

    // Build the probe once; the wire content does not change across the
    // three sends of this phase.
    UidProbePacket probe{};
    probe.header.type = PacketType::UID_PROBE;
    probe.header.version = 0;
    probe.header.flags = 0;
    probe.header.timestamp = 0;
    probe.header.prev_delay = 0;
    probe.packet_data.candidate_uid = candidate;
    probe.packet_data.src_mac = m_mac_u64;
    probe.packet_data.salt = salt;
    std::memset(probe.packet_data.__padding__, 0,
                sizeof(probe.packet_data.__padding__));

    const uint64_t deadline_us = m_clock.now_us()
        + m_tunables.probe_count * m_tunables.probe_interval_us;
    uint32_t sends_done = 0;
    uint64_t next_send_us = m_clock.now_us();

    // Buffer big enough for any LowLatPacket<T> we might see on discovery.
    // The wire-level T is an OANPacket<...>, not its inner data struct.
    // MappingPacket is the largest of {MappingPacket, UidProbePacket,
    // UidDefendPacket}, so size to it.
    uint8_t buf[sizeof(LowLatPacket<MappingPacket>)];

    while (m_clock.now_us() < deadline_us) {
        // Send if it's time.
        if (sends_done < m_tunables.probe_count
            && m_clock.now_us() >= next_send_us) {
            int sent = m_sock.send_probe(probe);
            if (sent < 0) return false;
            ++sends_done;
            next_send_us += m_tunables.probe_interval_us;
        }

        // Recv with a short timeout so we revisit the send schedule.
        uint64_t now = m_clock.now_us();
        uint64_t until = std::min<uint64_t>(next_send_us, deadline_us);
        int timeout_ms = (until > now)
            ? static_cast<int>((until - now + 999) / 1000)
            : 0;
        if (timeout_ms <= 0) timeout_ms = 1;

        int rx = m_sock.recv_next(buf, sizeof(buf), timeout_ms);
        if (rx <= 0) continue; // timeout (0) or error (<0) — keep probing
        if (static_cast<size_t>(rx) < sizeof(ethhdr) + sizeof(LowLatHeader)
                                       + sizeof(CommonHeader)) {
            continue;
        }

        // Discover the payload type. LowLatPacket layout is
        // [ethhdr][LowLatHeader][T], and every T we care about starts with
        // CommonHeader.
        const CommonHeader* hdr = reinterpret_cast<const CommonHeader*>(
            buf + sizeof(ethhdr) + sizeof(LowLatHeader));

        switch (hdr->type) {
        case PacketType::UID_DEFEND: {
            if (static_cast<size_t>(rx) < sizeof(LowLatPacket<UidDefendPacket>)) break;
            const auto* def = reinterpret_cast<const LowLatPacket<UidDefendPacket>*>(buf);
            if (def->payload.packet_data.defended_uid == candidate
                && !macs_equal(m_mac, def->payload.packet_data.src_mac)) {
                out_conflict = true;
            }
            break;
        }
        case PacketType::UID_PROBE: {
            // Concurrent prober for our candidate. Treat as conflict only
            // if its src_mac differs from ours. The other side will see
            // OUR probe too and the lower-saltwalk-overhead side (i.e.
            // whichever boots first per ALOHA) becomes incumbent.
            if (static_cast<size_t>(rx) < sizeof(LowLatPacket<UidProbePacket>)) break;
            const auto* p = reinterpret_cast<const LowLatPacket<UidProbePacket>*>(buf);
            if (p->payload.packet_data.candidate_uid == candidate
                && !macs_equal(m_mac, p->payload.packet_data.src_mac)) {
                out_conflict = true;
            }
            break;
        }
        case PacketType::MAPPING: {
            if (static_cast<size_t>(rx) < sizeof(LowLatPacket<MappingPacket>)) break;
            const auto* m = reinterpret_cast<const LowLatPacket<MappingPacket>*>(buf);
            if (m->payload.packet_data.self_uid == candidate
                && !macs_equal(m_mac, m->payload.packet_data.self_address)) {
                out_conflict = true;
            }
            break;
        }
        default:
            break;
        }

        if (out_conflict) return true;
    }

    return true; // probe phase completed; out_conflict reflects what we saw
}
