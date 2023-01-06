#include <iostream>

#include "platform/platform.h"

#include "config.h"


bool checkAvrPresent() {
    return i2c::transmit(I2C_ADDRESS, nullptr, 0, nullptr, 0);
}

bool enterBootloader() {
    return false;
}

int main(int argc, char * argv[]) {
    // initialize gpio and i2c
    gpio::initialize();
    i2c::initializeMaster();
    // check if we have AVR ready
    if (! checkAvrPresent()) {
        std::cerr << "AVR Chip not detected at I2C address " << I2C_ADDRESS << std::endl;
        return EXIT_FAILURE;
    }
    // enter the bootloader
    if (! enterBootloader()) {
        std::cerr << "Cannot enter AVR's bootloader" << std::endl;
        return EXIT_FAILURE;
    }
}