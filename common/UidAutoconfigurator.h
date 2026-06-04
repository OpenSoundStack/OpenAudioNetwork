// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#ifndef OPENAUDIONETWORK_UID_AUTOCONFIGURATOR_H
#define OPENAUDIONETWORK_UID_AUTOCONFIGURATOR_H

#include <cstdint>

#include "IDiscoverySocket.h"
#include "IUidClock.h"
#include "UidStore.h"

/**
 * @struct UidAutoconfigResult
 * @brief Outcome of one boot-time autoconfiguration run.
 */
struct UidAutoconfigResult {
    uint16_t committed_uid;     /**< The UID this peer now holds */
    uint8_t  salt_used;         /**< Salt that produced committed_uid */
    bool     used_persisted;    /**< True iff committed_uid came from the store */
    bool     had_collision;     /**< True iff any salt step saw a conflict */
};

/**
 * @class UidAutoconfigurator
 * @brief Boot-time UID claim algorithm per Docs/proposals/uid-autoconfiguration.md.
 *
 * Synchronous and blocking. Called once at boot, before any other OAN
 * traffic flows from this peer. The algorithm:
 *
 *   1. Honour static-range hint if present (return immediately).
 *   2. Otherwise try the persisted UID first.
 *   3. ALOHA jitter [0, 250ms], then probe phase (3 probes / ~750ms).
 *   4. On conflict, salt-walk (persisted UID gets one shot; on contest,
 *      restart salt walk from 0).
 *   5. Commit, persist, return.
 *
 * The algorithm does not start the runtime defence loop — that lives
 * in NetworkMapper after commit.
 */
class UidAutoconfigurator {
public:
    /**
     * @param self_mac      6-byte MAC of this peer (low 48 bits used in hash)
     * @param hint_uid      Optional hint from local config; honoured iff in
     *                      the static range [0xF000, 0xFFFE]. Dynamic-range
     *                      hints are ignored with a warning. Pass 0 for
     *                      "no hint".
     * @param store         Persistence backing (file, JSON field, env, null)
     * @param sock          Discovery-socket seam (real LowLatSocket or fake)
     * @param clock         Time + RNG source (RealUidClock or fake)
     */
    UidAutoconfigurator(const uint8_t self_mac[6],
                        uint16_t hint_uid,
                        IUidStore& store,
                        IDiscoverySocket& sock,
                        IUidClock& clock);

    UidAutoconfigResult run();

    // Exposed for unit tests and for diagnostic logging. Stable contract:
    // FNV-1a over the 6 MAC bytes (big-endian as provided) followed by the
    // 1-byte salt, then folded to 16 bits and clamped to [0x0001, 0xEFFF].
    static uint16_t hash16(const uint8_t mac[6], uint8_t salt);

    // UID space partition helpers (single source of truth).
    static constexpr uint16_t UID_BROADCAST    = 0x0000;
    static constexpr uint16_t UID_DYNAMIC_LO   = 0x0001;
    static constexpr uint16_t UID_DYNAMIC_HI   = 0xEFFF;
    static constexpr uint16_t UID_STATIC_LO    = 0xF000;
    static constexpr uint16_t UID_STATIC_HI    = 0xFFFE;
    static constexpr uint16_t UID_RESERVED     = 0xFFFF;

    static bool is_dynamic(uint16_t uid) {
        return uid >= UID_DYNAMIC_LO && uid <= UID_DYNAMIC_HI;
    }
    static bool is_static(uint16_t uid) {
        return uid >= UID_STATIC_LO && uid <= UID_STATIC_HI;
    }

    // Tunables — exposed for test override only. Defaults match the design.
    struct Tunables {
        uint32_t jitter_max_us = 250'000;   // ALOHA window
        uint32_t probe_count = 3;
        uint32_t probe_interval_us = 250'000; // 3 × 250ms = 750ms probe phase
        uint8_t  max_salt = 32;             // bail-out cap; design has no formal limit
    };
    void set_tunables(const Tunables& t) { m_tunables = t; }

private:
    bool probe_candidate(uint16_t candidate, uint8_t salt, bool& out_conflict);

    uint8_t  m_mac[6]{};
    uint64_t m_mac_u64{0};   // low 48 bits, MAC packed for wire
    uint16_t m_hint;
    IUidStore& m_store;
    IDiscoverySocket& m_sock;
    IUidClock& m_clock;
    Tunables m_tunables{};
};

#endif // OPENAUDIONETWORK_UID_AUTOCONFIGURATOR_H
