// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#ifndef LOWLATSOCKET_H
#define LOWLATSOCKET_H

#ifdef __linux__
#include "platforms/lls_linux.h"
#elif __ZEPHYR__
#include "platforms/lls_zephyr.h"
#endif

#endif //LOWLATSOCKET_H
