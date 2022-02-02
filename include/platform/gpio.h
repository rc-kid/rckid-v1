#pragma once
#include "platform.h"

#if (defined ARCH_RP2040)
#include <hardware/gpio.h>
#elif (defined ARCH_ARDUINO)
#else
ARCH_NOT_SUPPORTED;
#endif

namespace gpio {

#if (defined ARCH_RP2040)
    using Pin = unsigned;
    static constexpr Pin UNUSED = 0xffff;
#elif (defined ARCH_ARDUINO) 
    using Pin = int;
    static constexpr Pin UNUSED = -1;
#endif

    inline void initialize() {
#if (defined ARCH_RP2040)
        stdio_init_all();
#endif
    }

    inline void output(unsigned pin) {
#if (defined ARCH_RP2040)
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
#elif (defined ARCH_ARDUINO)
        pinMode(pin, OUTPUT);
#endif
    }

    inline void input(unsigned pin) {
#if (defined ARCH_RP2040)
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
#elif (defined ARCH_ARDUINO)
        pinMode(pin, INPUT);
#endif
    }

    inline void high(unsigned pin) {
#if (defined ARCH_RP2040)
        gpio_put(pin, true);
#elif (defined ARCH_ARDUINO)
        digitalWrite(pin, HIGH);
#endif
    }

    inline void low(unsigned pin) {
#if (defined ARCH_RP2040)
        gpio_put(pin, false);
#elif (defined ARCH_ARDUINO)
        digitalWrite(pin, LOW);
#endif
    }

} // namespace gpio