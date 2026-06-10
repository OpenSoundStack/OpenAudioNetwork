// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Jonathan D Reichardt
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0


#ifndef OPENAUDIONETWORK_RT_H
#define OPENAUDIONETWORK_RT_H
#include <cstdint>


namespace oals::rt {
    void set_thread_realtime(uint8_t);
    void set_running_cpu(int);
    void set_process_scheduler_rr(int);
    void precise_sleep(long);
}


#endif //OPENAUDIONETWORK_RT_H
