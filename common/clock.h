// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#ifndef CLOCK_H
#define CLOCK_H

#include <cstdint>

enum ClockSyncState : uint8_t {
    CKSYNC_NO_SYNC,
    CKSYNC_SYNC,
    CKSYNC_DELAY_REQ,
    CKSYNC_DELAY_RESP
};

#endif //CLOCK_H
