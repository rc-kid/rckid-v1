#pragma once
#include "platform.h"

#if (defined ARCH_RP2040)
#include <hardware/gpio.h>
#elif (defined ARCH_ARDUINO)
#elif (defined ARCH_RPI)
#include <pigpio.h>
#elif (defined ARCH_MOCK)
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
#elif (defined ARCH_RPI)
        gpioInitialise();
#endif
    }

    inline void output(unsigned pin) {
#if (defined ARCH_RP2040)
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
#elif (defined ARCH_ARDUINO)
        pinMode(pin, OUTPUT);
#elif (defined ARCH_RPI)
        gpioSetMode(pin, PI_OUTPUT);
#endif
    }

    inline void input(unsigned pin) {
#if (defined ARCH_RP2040)
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
#elif (defined ARCH_ARDUINO)
        pinMode(pin, INPUT);
#elif (defined ARCH_RPI)
        gpioSetMode(pin, PI_INPUT);
#endif
    }


    inline void inputPullup(unsigned pin) {
#if (defined ARCH_RP2040)
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
#elif (defined ARCH_ARDUINO)
        pinMode(pin, INPUT_PULLUP);
#elif (defined ARCH_RPI)
        gpioSetMode(pin, PI_OUTPUT);
        gpioSetPullUpDown(pin, PI_PUD_UP);
#endif
    }

    inline void high(unsigned pin) {
#if (defined ARCH_RP2040)
        gpio_put(pin, true);
#elif (defined ARCH_ARDUINO)
        digitalWrite(pin, HIGH);
#elif (defined ARCH_RPI)
        gpioWrite(pin, 1);
#endif
    }

    inline void low(unsigned pin) {
#if (defined ARCH_RP2040)
        gpio_put(pin, false);
#elif (defined ARCH_ARDUINO)
        digitalWrite(pin, LOW);
#elif (defined ARCH_RPI)
        gpioWrite(pin, 0);
#endif
    }

    inline bool read(unsigned pin) {
#if (defined ARCH_RP2040)
        return gpio_get(pin);
#elif (defined ARCH_ARDUINO)
        return digitalRead(pin);
#elif (defined ARCH_RPI)
        return gpioRead(pin) == 1;
#endif
        return 0;
    }

} // namespace gpio