#include "ili9341.h"

void ILI9341::initialize() {
    // set the pins
    gpio_init(CSX_PIN);
    gpio_set_dir(CSX_PIN, GPIO_OUT);
    gpio_put(CSX_PIN, true);
    gpio_init(DCX_PIN);
    gpio_set_dir(DCX_PIN, GPIO_OUT);
    gpio_put(DCX_PIN, true);
    // initialize data pins to mcu

    initializePin(CSX_PIN);
    initializePin(DCX_PIN);
    initializePin(WRX_PIN);
    for (unsigned i = 0; i < 8; ++i)
        initializePin(DATA_START_PIN + i);
    // now load the PIO driver
}