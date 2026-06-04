// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#include "IUidClock.h"

#include <chrono>
#include <thread>

RealUidClock::RealUidClock() {
    std::random_device rd;
    m_rng.seed(rd());
}

uint64_t RealUidClock::now_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

void RealUidClock::sleep_us(uint64_t us) {
    std::this_thread::sleep_for(std::chrono::microseconds(us));
}

uint32_t RealUidClock::random_u32() {
    return static_cast<uint32_t>(m_rng());
}
