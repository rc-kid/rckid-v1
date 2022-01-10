#pragma once
#include "platform.h"

#if (defined ARCH_RP2040)
#include <hardware/gpio.h>
#else
ARCH_NOT_SUPPORTED;
#endif

namespace gpio {

    using Pin = unsigned;

    inline void output(unsigned pin) {
#if (defined ARCH_RP2040)
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
#endif
    }

    inline void input(unsigned pin) {
#if (defined ARCH_RP2040)
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
#endif
    }

    inline void high(unsigned pin) {
#if (defined ARCH_RP2040)
        gpio_put(pin, true);
#endif
    }

    inline void low(unsigned pin) {
#if (defined ARCH_RP2040)
        gpio_put(pin, false);
#endif
    }

} // namespace gpio