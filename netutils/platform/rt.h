// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#ifndef OALS_RT_H
#define OALS_RT_H

#include <cstdint>

namespace oals::rt {
    void set_thread_realtime(uint8_t prio);
    void set_running_cpu(int cpu_id);
    void set_process_scheduler_rr(int prio);
    void precise_sleep_ns(long ns);
}

#endif
