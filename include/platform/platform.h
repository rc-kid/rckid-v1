#pragma once
#include <stdint.h>

#if (defined ARDUINO)
    #define ARCH_ARDUINO
    #include <Arduino.h>
#endif

#if (defined PICO_BOARD)
    #define ARCH_RP2040
#elif (defined __AVR_ATtiny1616__)
    #define ARCH_AVR_MEGATINY
    #define ARCH_ATTINY_1616
#elif  (defined __AVR_ATtiny3216)
    #define ARCH_AVR_MEGATINY
    #define ARCH_ATTINY_3216
#elif (defined __AVR_ATmega8__)
    #define ARCH_AVR_MEGA
#endif

#define ARCH_NOT_SUPPORTED static_assert(false,"Unknown or unsupported architecture")


inline uint16_t swapBytes(uint16_t x) {
    return static_cast<uint16_t>((x & 0xff) << 8 | (x >> 8));
}

template<typename T, typename V>
void setOrClear(T & value, V mask, bool setOrClear) {
    if (setOrClear)
        value |= mask;
    else 
        value &= ~mask;
}

namespace cpu {

    inline void delay_us(unsigned value) {
#if (defined ARCH_RP2040)
        sleep_us(value);  
#elif (defined ARCH_ARDUINO)
        delayMicroseconds(value);
#else
        ARCH_NOT_SUPPORTED;
#endif
    }

    inline void delay_ms(unsigned value) {
#if (defined ARCH_RP2040)
        sleep_ms(value);  
#elif (defined ARCH_ARDUINO)
        delay(value);
#else
        ARCH_NOT_SUPPORTED;
#endif
    }
}