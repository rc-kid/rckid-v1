#pragma once
#include "platform.h"
#include "gpio.h"

#if (defined ARCH_RP2040)
#include <hardware/spi.h>
#include <pico/binary_info.h>
#else
ARCH_NOT_SUPPORTED;
#endif


namespace spi {

#if (defined ARCH_RP2040)
    inline void initialize(unsigned miso, unsigned mosi, unsigned sck) {
        spi_init(spi0, 5000000); // init spi0 at 5Mhz
        gpio_set_function(miso, GPIO_FUNC_SPI);
        gpio_set_function(mosi, GPIO_FUNC_SPI);
        gpio_set_function(sck, GPIO_FUNC_SPI);
        // Make the SPI pins available to picotool
        bi_decl(bi_3pins_with_func(miso, mosi, sck, GPIO_FUNC_SPI));        
    }
#else
    inline void initialize() {
        ARCH_NOT_SUPPORTED;
    }
#endif

    void setCs(gpio::Pin cs, bool value) {
#if (defined ARCH_RP2040)
        asm volatile("nop \n nop \n nop");
        value ? gpio::low(cs) : gpio::high(cs);
        asm volatile("nop \n nop \n nop");        
#else
        value ? gpio::low(cs) : gpio::high(cs);
#endif
    }

    inline uint8_t transfer(uint8_t value) {
#if (defined ARCH_RP2040)
        uint8_t result;
        spi_read_blocking(spi0, value, & result, 1);
        return result;
#endif
    }

} // namespace spi