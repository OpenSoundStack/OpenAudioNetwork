// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Jonathan D Reichardt
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0


#include "rt.h"


#ifdef __linux__
#include <iostream>
#include <sched.h>
#include <ctime>
#else
#include <ctime>
#endif

namespace oals::rt {
#ifdef __linux__
    void set_thread_realtime(uint8_t prio) {
        sched_param sparams{};
        sparams.sched_priority = prio;

        if (sched_setscheduler(0, SCHED_FIFO, &sparams) != 0) {
            std::cerr << "[oals::rt] set_thread_realtime failed" << std::endl;
        }
    }
    void set_running_cpu(int cpu_id) {
        cpu_set_t cs{};
        CPU_ZERO(&cs);
        CPU_SET(cpu_id, &cs);

        if (sched_setaffinity(0, sizeof(cpu_set_t), &cs) != 0) {
            std::cerr << "[oals::rt] set_running_cpu failed" << std::endl;
        }
    }

    void set_process_scheduler_rr(int prio) {
        sched_param params{};
        params.sched_priority = prio;
        if (sched_setscheduler(0, SCHED_RR, &params) != 0) {
            std::cerr << "[oals::rt] set_process_scheduler_rr failed" << std::endl;
        }
    }

    void precise_sleep(long ns) {
        timespec ts{};
        ts.tv_sec = ns / 1'000'000'000;
        ts.tv_nsec = ns % 1'000'000'000;
        clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, nullptr);
    }


#else
    void set_thread_realtime(uint8_t) {
    }

    void set_running_cpu(int) {
    }

    void set_process_scheduler_rr(int) {
    }

    void precise_sleep(long ns) {
        timespec ts{};
        ts.tv_sec = ns / 1'000'000'000;
        ts.tv_nsec = ns % 1'000'000'000;
        nanosleep(&ts, nullptr);
    }
#endif
}
