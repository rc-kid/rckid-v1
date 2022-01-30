#pragma once

#include <inttypes.h>


#include "platform.h"

#if (defined ARCH_RP2040)

/** A simple RAII timer with [us] resolution.
 */
class DebugTimer {
public:
    DebugTimer(char const * name = nullptr):
        name_{name},
        start_{get_absolute_time()} {
    }

    ~DebugTimer() {
        auto end = get_absolute_time();
        if (name_ != nullptr)
            printf("%s: %" PRId64 " [us]\n", name_, absolute_time_diff_us(start_, end));
        else
            printf("Elapsed time: %" PRId64 " [us]\n", absolute_time_diff_us(start_, end));
    }

private:
    char const * const name_ = nullptr;
    absolute_time_t start_;
}; // DebugTimer


#endif