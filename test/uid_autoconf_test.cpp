// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0
//
// Self-contained unit tests for UidAutoconfigurator + FileUidStore.
// No GoogleTest dependency — matches the existing OpenDSP test style.
// Returns non-zero exit code if anything fails.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "common/IDiscoverySocket.h"
#include "common/IUidClock.h"
#include "common/UidAutoconfigurator.h"
#include "common/UidStore.h"
#include "common/packet_structs.h"
#include "netutils/LowLatSocket.h"

namespace {

int g_failures = 0;
int g_assertions = 0;

#define CHECK(cond) do { \
    ++g_assertions; \
    if (!(cond)) { \
        ++g_failures; \
        std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << ": " #cond << std::endl; \
    } \
} while (0)

#define CHECK_EQ(a, b) do { \
    ++g_assertions; \
    auto _av = (a); auto _bv = (b); \
    if (!(_av == _bv)) { \
        ++g_failures; \
        std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ \
                  << ": " #a " == " #b " (got " << _av << " vs " << _bv << ")" << std::endl; \
    } \
} while (0)

// --- Fakes -----------------------------------------------------------------

// Deterministic clock: now_us advances only via sleep_us. random_u32 returns a
// scripted sequence (defaults to 0, so jitter is 0 by default).
class FakeClock : public IUidClock {
public:
    uint64_t now_us() override { return m_now; }
    void sleep_us(uint64_t us) override { m_now += us; }
    uint32_t random_u32() override {
        if (m_rng_queue.empty()) return 0;
        uint32_t v = m_rng_queue.front();
        m_rng_queue.pop_front();
        return v;
    }
    void push_random(uint32_t v) { m_rng_queue.push_back(v); }
    uint64_t m_now{0};
    std::deque<uint32_t> m_rng_queue;
};

// Scripted discovery socket: records probes sent, returns scripted packets
// from a queue when recv_next is called.
struct ScriptedRx {
    std::vector<uint8_t> bytes; // full LowLatPacket bytes
};

class FakeSocket : public IDiscoverySocket {
public:
    explicit FakeSocket(FakeClock* clock = nullptr) : m_clock(clock) {}

    int send_probe(const UidProbePacket& pkt) override {
        sent_probes.push_back(pkt);
        return sizeof(pkt);
    }
    int send_defend(const UidDefendPacket& pkt) override {
        sent_defends.push_back(pkt);
        return sizeof(pkt);
    }
    int recv_next(uint8_t* buf, size_t buf_len, int timeout_ms) override {
        if (rx_queue.empty()) {
            // Consume the timeout against the fake clock so the algorithm's
            // deadline-driven loop actually progresses in tests.
            if (m_clock && timeout_ms > 0) {
                m_clock->sleep_us(static_cast<uint64_t>(timeout_ms) * 1000);
            }
            return 0;
        }
        ScriptedRx s = std::move(rx_queue.front());
        rx_queue.pop_front();
        size_t n = std::min(buf_len, s.bytes.size());
        std::memcpy(buf, s.bytes.data(), n);
        return static_cast<int>(n);
    }

    FakeClock* m_clock;

    // Helpers to script inbound packets.
    template<class T>
    void queue_lowlat_packet(uint16_t sender_uid, uint64_t sender_mac,
                             const T& payload) {
        ScriptedRx s;
        s.bytes.resize(sizeof(LowLatPacket<T>));
        auto* llp = reinterpret_cast<LowLatPacket<T>*>(s.bytes.data());
        std::memset(llp, 0, sizeof(*llp));
        std::memcpy(llp->eth_header.h_dest, "\xff\xff\xff\xff\xff\xff", 6);
        // Pack sender_mac low-48 into h_source bytes.
        for (int i = 0; i < 6; ++i) {
            llp->eth_header.h_source[5 - i] =
                static_cast<uint8_t>((sender_mac >> (i * 8)) & 0xFF);
        }
        llp->llhdr.sender_uid = sender_uid;
        llp->llhdr.dest_uid = 0;
        llp->llhdr.psize = sizeof(T);
        llp->payload = payload;
        rx_queue.push_back(std::move(s));
    }

    std::vector<UidProbePacket> sent_probes;
    std::vector<UidDefendPacket> sent_defends;
    std::deque<ScriptedRx> rx_queue;
};

UidProbePacket make_probe(uint16_t cand, uint64_t src_mac, uint8_t salt = 0) {
    UidProbePacket p{};
    p.header.type = PacketType::UID_PROBE;
    p.packet_data.candidate_uid = cand;
    p.packet_data.src_mac = src_mac;
    p.packet_data.salt = salt;
    return p;
}

UidDefendPacket make_defend(uint16_t uid, uint64_t src_mac) {
    UidDefendPacket p{};
    p.header.type = PacketType::UID_DEFEND;
    p.packet_data.defended_uid = uid;
    p.packet_data.src_mac = src_mac;
    p.packet_data.since_us = 0;
    return p;
}

MappingPacket make_mapping(uint16_t self_uid, uint64_t self_mac) {
    MappingPacket p{};
    p.header.type = PacketType::MAPPING;
    p.packet_data.self_uid = self_uid;
    p.packet_data.self_address = self_mac;
    return p;
}

uint64_t mac_to_u64(const uint8_t mac[6]) {
    uint64_t v = 0;
    for (int i = 0; i < 6; ++i) v = (v << 8) | mac[i];
    return v;
}

UidAutoconfigurator::Tunables fast_tunables() {
    UidAutoconfigurator::Tunables t;
    t.jitter_max_us = 0;       // deterministic; no jitter
    t.probe_count = 3;
    t.probe_interval_us = 1000; // 1 ms per probe — fast tests
    t.max_salt = 8;
    return t;
}

// --- Tests -----------------------------------------------------------------

void test_hash_determinism() {
    uint8_t mac_a[6] = {0x02, 0x00, 0x00, 0x12, 0x34, 0x56};
    uint8_t mac_b[6] = {0x02, 0x00, 0x00, 0x12, 0x34, 0x57};

    uint16_t a0 = UidAutoconfigurator::hash16(mac_a, 0);
    uint16_t a0_again = UidAutoconfigurator::hash16(mac_a, 0);
    CHECK_EQ(a0, a0_again);

    uint16_t b0 = UidAutoconfigurator::hash16(mac_b, 0);
    CHECK(a0 != b0); // different MACs → different candidates (statistically)

    uint16_t a1 = UidAutoconfigurator::hash16(mac_a, 1);
    CHECK(a0 != a1); // different salts → different candidates

    // All outputs must land in dynamic range.
    CHECK(UidAutoconfigurator::is_dynamic(a0));
    CHECK(UidAutoconfigurator::is_dynamic(a1));
    CHECK(UidAutoconfigurator::is_dynamic(b0));
}

void test_hash_distribution_sanity() {
    // Coarse: 256 distinct MACs should mostly land on distinct UIDs.
    // We allow some collisions (birthday paradox) but expect "most" unique.
    std::vector<uint16_t> v;
    v.reserve(256);
    for (int i = 0; i < 256; ++i) {
        uint8_t mac[6] = {0x02, 0x00, 0x00, 0x00,
                          static_cast<uint8_t>(i >> 8),
                          static_cast<uint8_t>(i & 0xFF)};
        v.push_back(UidAutoconfigurator::hash16(mac, 0));
    }
    std::sort(v.begin(), v.end());
    size_t dupes = 0;
    for (size_t i = 1; i < v.size(); ++i) {
        if (v[i] == v[i - 1]) ++dupes;
    }
    // For 256 inputs into 61439 buckets, expected ~0.5 collisions.
    // Anything >5 is suspicious.
    CHECK(dupes < 5);
}

void test_happy_path_commits_canonical() {
    uint8_t mac[6] = {0x02, 0x00, 0x00, 0x11, 0x22, 0x33};
    NullUidStore store;
    FakeClock clk;
    FakeSocket sock(&clk);

    UidAutoconfigurator cfg(mac, /*hint=*/0, store, sock, clk);
    cfg.set_tunables(fast_tunables());

    auto res = cfg.run();
    CHECK_EQ(res.committed_uid, UidAutoconfigurator::hash16(mac, 0));
    CHECK_EQ(res.salt_used, 0);
    CHECK_EQ(res.used_persisted, false);
    CHECK_EQ(res.had_collision, false);
    CHECK_EQ(sock.sent_probes.size(), 3u);
}

void test_persisted_uid_honoured() {
    uint8_t mac[6] = {0x02, 0x00, 0x00, 0x44, 0x55, 0x66};
    FakeClock clk;
    FakeSocket sock(&clk);

    // FileUidStore in a temp path with a pre-seeded UID.
    auto tmp = std::filesystem::temp_directory_path() / "oals_uid_test_persisted.txt";
    std::filesystem::remove(tmp);
    { std::ofstream f(tmp); f << "31337\n"; }

    FileUidStore store(tmp.string());
    UidAutoconfigurator cfg(mac, 0, store, sock, clk);
    cfg.set_tunables(fast_tunables());

    auto res = cfg.run();
    CHECK_EQ(res.committed_uid, 31337);
    CHECK_EQ(res.used_persisted, true);
    CHECK_EQ(res.had_collision, false);
    // Store should still contain 31337 after the commit save.
    auto loaded = store.load();
    CHECK(loaded.has_value());
    CHECK_EQ(*loaded, 31337);

    std::filesystem::remove(tmp);
}

void test_persisted_uid_contested_restarts_salt_walk() {
    uint8_t mac[6] = {0x02, 0x00, 0x00, 0x77, 0x88, 0x99};
    FakeClock clk;
    FakeSocket sock(&clk);
    NullUidStore null_store;

    // Use a FileUidStore so we can verify clear() happens on contest.
    auto tmp = std::filesystem::temp_directory_path() / "oals_uid_test_contested.txt";
    std::filesystem::remove(tmp);
    { std::ofstream f(tmp); f << "12345\n"; }
    FileUidStore store(tmp.string());

    // Queue a defend for 12345 from some other MAC; this should make the
    // configurator give up on the persisted UID, clear the store, restart
    // salt walk from 0, and commit hash16(mac, 0).
    sock.queue_lowlat_packet(0, 0xDEADBEEFCAFEull,
                             make_defend(12345, 0xDEADBEEFCAFEull));

    UidAutoconfigurator cfg(mac, 0, store, sock, clk);
    cfg.set_tunables(fast_tunables());
    auto res = cfg.run();

    CHECK_EQ(res.used_persisted, false);
    CHECK_EQ(res.had_collision, true);
    CHECK_EQ(res.committed_uid, UidAutoconfigurator::hash16(mac, 0));
    CHECK_EQ(res.salt_used, 0);

    // Store now contains the new committed UID, not the original 12345.
    auto loaded = store.load();
    CHECK(loaded.has_value());
    CHECK_EQ(*loaded, UidAutoconfigurator::hash16(mac, 0));

    std::filesystem::remove(tmp);
}

void test_single_collision_salt_walk() {
    uint8_t mac[6] = {0x02, 0x00, 0x00, 0xAA, 0xBB, 0xCC};
    FakeClock clk;
    FakeSocket sock(&clk);
    NullUidStore store;

    uint16_t cand0 = UidAutoconfigurator::hash16(mac, 0);

    // Defend cand0 from a different MAC → forces salt walk to 1.
    sock.queue_lowlat_packet(0, 0x010203040506ull, make_defend(cand0, 0x010203040506ull));

    UidAutoconfigurator cfg(mac, 0, store, sock, clk);
    cfg.set_tunables(fast_tunables());
    auto res = cfg.run();

    CHECK_EQ(res.had_collision, true);
    CHECK_EQ(res.salt_used, 1);
    CHECK_EQ(res.committed_uid, UidAutoconfigurator::hash16(mac, 1));
}

void test_mapping_packet_also_triggers_conflict() {
    uint8_t mac[6] = {0x02, 0x00, 0x00, 0xDE, 0xAD, 0x01};
    FakeClock clk;
    FakeSocket sock(&clk);
    NullUidStore store;

    uint16_t cand0 = UidAutoconfigurator::hash16(mac, 0);
    // A real-world peer announcing itself via MappingPacket with our candidate
    // UID from a different MAC should be treated as conflict.
    sock.queue_lowlat_packet(cand0, 0x0BADBADC0FEEull,
                             make_mapping(cand0, 0x0BADBADC0FEEull));

    UidAutoconfigurator cfg(mac, 0, store, sock, clk);
    cfg.set_tunables(fast_tunables());
    auto res = cfg.run();

    CHECK_EQ(res.had_collision, true);
    CHECK_EQ(res.salt_used, 1);
}

void test_static_hint_bypasses_algorithm() {
    uint8_t mac[6] = {0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF};
    FakeClock clk;
    FakeSocket sock(&clk);
    NullUidStore store;

    UidAutoconfigurator cfg(mac, /*hint=*/0xF010, store, sock, clk);
    cfg.set_tunables(fast_tunables());
    auto res = cfg.run();

    CHECK_EQ(res.committed_uid, 0xF010);
    CHECK_EQ(res.had_collision, false);
    CHECK_EQ(sock.sent_probes.size(), 0u); // no probe phase for static UIDs
}

void test_dynamic_hint_ignored_with_warning() {
    uint8_t mac[6] = {0x02, 0x00, 0x00, 0x01, 0x02, 0x03};
    FakeClock clk;
    FakeSocket sock(&clk);
    NullUidStore store;

    // 100 is dynamic-range → ignored.
    UidAutoconfigurator cfg(mac, /*hint=*/100, store, sock, clk);
    cfg.set_tunables(fast_tunables());
    auto res = cfg.run();

    CHECK_EQ(res.committed_uid, UidAutoconfigurator::hash16(mac, 0));
}

void test_file_store_roundtrip() {
    auto tmp = std::filesystem::temp_directory_path() / "oals_uid_test_rt.txt";
    std::filesystem::remove(tmp);

    FileUidStore s(tmp.string());
    CHECK(!s.load().has_value());

    s.save(42);
    auto loaded = s.load();
    CHECK(loaded.has_value());
    CHECK_EQ(*loaded, 42);

    s.clear();
    CHECK(!s.load().has_value());
}

void test_file_store_malformed() {
    auto tmp = std::filesystem::temp_directory_path() / "oals_uid_test_bad.txt";
    std::filesystem::remove(tmp);
    { std::ofstream f(tmp); f << "not-a-number"; }
    FileUidStore s(tmp.string());
    CHECK(!s.load().has_value());
    std::filesystem::remove(tmp);
}

void test_env_override_pin() {
    auto tmp = std::filesystem::temp_directory_path() / "oals_uid_test_env.txt";
    std::filesystem::remove(tmp);
    { std::ofstream f(tmp); f << "1\n"; }

    setenv("OALS_TEST_UID_ENV", "9999", 1);
    EnvOverrideUidStore s("OALS_TEST_UID_ENV",
                          std::make_unique<FileUidStore>(tmp.string()));
    auto loaded = s.load();
    CHECK(loaded.has_value());
    CHECK_EQ(*loaded, 9999); // env wins over file
    s.save(7777); // should be a no-op because env is set
    // Direct file read confirms we did not overwrite.
    FileUidStore raw(tmp.string());
    auto raw_loaded = raw.load();
    CHECK(raw_loaded.has_value());
    CHECK_EQ(*raw_loaded, 1);

    unsetenv("OALS_TEST_UID_ENV");
    std::filesystem::remove(tmp);
}

} // namespace

int main() {
    test_hash_determinism();
    test_hash_distribution_sanity();
    test_happy_path_commits_canonical();
    test_persisted_uid_honoured();
    test_persisted_uid_contested_restarts_salt_walk();
    test_single_collision_salt_walk();
    test_mapping_packet_also_triggers_conflict();
    test_static_hint_bypasses_algorithm();
    test_dynamic_hint_ignored_with_warning();
    test_file_store_roundtrip();
    test_file_store_malformed();
    test_env_override_pin();

    std::cout << "uid_autoconf_test: " << g_assertions << " assertions, "
              << g_failures << " failures." << std::endl;
    return g_failures == 0 ? 0 : 1;
}
