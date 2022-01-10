#pragma once
#include <cstdint>

#if (defined PICO_BOARD)
    #define ARCH_RP2040
#endif

#define ARCH_NOT_SUPPORTED static_assert(false,"Unknown or unsupported architecture")


namespace cpu {
    inline void delay_us(unsigned value) {
#if (defined ARCH_RP2040)
        sleep_us(value);  
#else
        ARCH_NOT_SUPPORTED;
#endif
    }
}