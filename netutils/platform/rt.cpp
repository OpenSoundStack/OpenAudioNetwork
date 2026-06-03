// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#include "rt.h"

#ifdef __linux__
  #include <iostream>
  #include <linux/sched.h>
  #include <sched.h>
  #include <time.h>
#elif defined(__APPLE__)
  #include <algorithm>
  #include <cerrno>
  #include <cstring>
  #include <iostream>
  #include <mach/mach.h>
  #include <mach/mach_time.h>
  #include <mach/thread_act.h>
  #include <mach/thread_policy.h>
  #include <pthread.h>
  #include <sys/mman.h>
  #include <time.h>
#else
  #include <time.h>
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

void precise_sleep_ns(long ns) {
    timespec ts{};
    ts.tv_sec  = ns / 1'000'000'000L;
    ts.tv_nsec = ns % 1'000'000'000L;
    clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, nullptr);
}

#elif defined(__APPLE__)

namespace {

// Cached host timebase: mach absolute ticks -> nanoseconds is
// (ticks * numer / denom). thread_time_constraint_policy_data_t takes
// values in absolute mach ticks, so anything we compute in ns has to go
// through this. Init-once, lock-free read after.
mach_timebase_info_data_t s_tb_info = {};
bool                      s_tb_init = false;

void ensure_timebase() {
    if (s_tb_init) return;
    mach_timebase_info(&s_tb_info);
    s_tb_init = true;
}

uint32_t ns_to_mach_ticks(uint64_t ns) {
    ensure_timebase();
    // ns * denom / numer — order matters to avoid overflow at audio-block
    // scale (period ~667 µs, computation a fraction of that).
    uint64_t ticks = (ns * s_tb_info.denom) / s_tb_info.numer;
    if (ticks > UINT32_MAX) ticks = UINT32_MAX;
    return static_cast<uint32_t>(ticks);
}

}  // namespace

// Pin the calling thread as a time-constraint thread. We map our Linux
// SCHED_FIFO priority (20..80) to a fraction of the audio block period:
// higher priority → larger computation budget within the same period.
// The audio block is 64 samples @ 96 kHz ≈ 667 µs; the pipe_updater
// (prio 80) and audio recv (prio 25) both want to finish well inside
// that. We pick:
//   period      = 667 µs (one audio block)
//   computation = (prio / 100) * period, clamped to [50 µs, 500 µs]
//   constraint  = period   (must complete within one period)
//   preemptible = true     (yield if we overrun — matches SCHED_FIFO behaviour)
//
// macOS scheduler treats this as "wake within `period`, give us up to
// `computation` of compute, must finish within `constraint`." Overrun
// doesn't kill us; the kernel just keeps running us at lower priority
// until we yield. That's the same shape as Linux SCHED_FIFO overrun.
//
// Refs: Apple TN2169, Chromium base/threading/platform_thread_mac.mm.
void set_thread_realtime(uint8_t prio) {
    constexpr uint64_t k_audio_block_ns = 667'000;  // 64 / 96000 * 1e9 ≈ 667 µs

    uint64_t computation_ns =
        static_cast<uint64_t>(k_audio_block_ns) * std::max<uint8_t>(prio, 5) / 100;
    if (computation_ns < 50'000)  computation_ns = 50'000;   // floor 50 µs
    if (computation_ns > 500'000) computation_ns = 500'000;  // ceil 500 µs (75% of period)

    thread_time_constraint_policy_data_t policy;
    policy.period      = ns_to_mach_ticks(k_audio_block_ns);
    policy.computation = ns_to_mach_ticks(computation_ns);
    policy.constraint  = ns_to_mach_ticks(k_audio_block_ns);
    policy.preemptible = TRUE;

    kern_return_t kr = thread_policy_set(
        pthread_mach_thread_np(pthread_self()),
        THREAD_TIME_CONSTRAINT_POLICY,
        reinterpret_cast<thread_policy_t>(&policy),
        THREAD_TIME_CONSTRAINT_POLICY_COUNT);

    if (kr != KERN_SUCCESS) {
        std::cerr << "[oals::rt] thread_policy_set(time_constraint) failed: "
                  << kr << std::endl;
    }
}

// thread_affinity_policy_set exists but is documented as a hint and is
// effectively a no-op on Apple Silicon (no P-core pinning). Skip — the
// scheduler does a better job placing time-constraint threads than we
// could anyway.
void set_running_cpu(int) {}

// On Linux this sets the whole process to SCHED_RR. On Mac we use the
// hook as the natural one-time-per-process spot for mlockall: paging out
// the engine's RT stacks while a show is running is the worst failure
// mode there is. mlockall is per-process so this is idempotent if a
// caller invokes us more than once.
void set_process_scheduler_rr(int) {
    static bool s_mlocked = false;
    if (s_mlocked) return;
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        std::cerr << "[oals::rt] mlockall failed (continuing): "
                  << strerror(errno) << std::endl;
    }
    s_mlocked = true;
}

void precise_sleep_ns(long ns) {
    timespec ts{};
    ts.tv_sec  = ns / 1'000'000'000L;
    ts.tv_nsec = ns % 1'000'000'000L;
    nanosleep(&ts, nullptr);
}

#else

void set_thread_realtime(uint8_t) {}
void set_running_cpu(int)         {}
void set_process_scheduler_rr(int){}

void precise_sleep_ns(long ns) {
    timespec ts{};
    ts.tv_sec  = ns / 1'000'000'000L;
    ts.tv_nsec = ns % 1'000'000'000L;
    nanosleep(&ts, nullptr);
}

#endif

}
