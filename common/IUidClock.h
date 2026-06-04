// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#ifndef OPENAUDIONETWORK_IUID_CLOCK_H
#define OPENAUDIONETWORK_IUID_CLOCK_H

#include <cstdint>
#include <random>

/**
 * @class IUidClock
 * @brief Time + jitter source for the UID autoconfigurator.
 *
 * Abstracted so unit tests can run the algorithm without burning real
 * wall-clock time on jitter and probe deadlines. The production
 * implementation is RealUidClock; tests substitute a deterministic
 * fake.
 */
struct IUidClock {
    virtual ~IUidClock() = default;

    /** Monotonic-ish microseconds since some fixed epoch. */
    virtual uint64_t now_us() = 0;

    /** Sleep for the given number of microseconds. */
    virtual void sleep_us(uint64_t us) = 0;

    /** Return an unsigned 32-bit random value (for jitter, salt seeding). */
    virtual uint32_t random_u32() = 0;
};

/**
 * @class RealUidClock
 * @brief std::chrono::steady_clock + std::this_thread::sleep_for +
 *        std::mt19937 seeded from std::random_device.
 */
class RealUidClock : public IUidClock {
public:
    RealUidClock();
    uint64_t now_us() override;
    void sleep_us(uint64_t us) override;
    uint32_t random_u32() override;

private:
    std::mt19937 m_rng;
};

#endif // OPENAUDIONETWORK_IUID_CLOCK_H
