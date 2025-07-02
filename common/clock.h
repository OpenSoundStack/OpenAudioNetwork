// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2025 - Mathis DELGADO
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, version 3 of the License.
//
// This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

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
