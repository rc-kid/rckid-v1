#include <iostream>

#include "platform/platform.h"

#include "config.h"

bool checkAvrPresent() {
    return i2c::transmit(I2C_ADDRESS, nullptr, 0, nullptr, 0);
    cpu::delay_ms(10);
}

void sendCommand(uint8_t cmd) {
    i2c::transmit(I2C_ADDRESS, & cmd, 1, nullptr, 0);
    cpu::delay_ms(10);
}

void sendCommand(uint8_t cmd, uint8_t arg) {
    uint8_t data[] = { cmd, arg};
    i2c::transmit(I2C_ADDRESS, data, 2, nullptr, 0);
    cpu::delay_ms(10);
}

void readBuffer(uint8_t * buffer) {
    i2c::transmit(I2C_ADDRESS, nullptr, 0, buffer, 32);
}

void writeBuffer(uint8_t const * buffer) {
    uint8_t data[33];
    data[0] = CMD_WRITE_BUFFER;
    for (size_t i = 0; i < 32; ++i)
        data[i+1] = buffer[i];
    i2c::transmit(I2C_ADDRESS, data, 33, nullptr, 0);
}


bool enterBootloader() {
    // to enter the bootloader, pull the AVR_IRQ pin low first
    // gpio::output()
    //
    // reset the AVR
    sendCommand(CMD_RESET);
    // verify that we have proper communications available by writing some stuff and then reading it back
    sendCommand(CMD_CLEAR_INDEX);
    char const * text = "ThisIsATest_DoYouHearMe?12345678";
    writeBuffer((uint8_t const *)text);
    uint8_t data[32];
    readBuffer(data);
    if (strncmp(text, (char const *)data, 32) != 0) {
        throw "IO Error. Device detected but I2C communication not working";
    }

    sendCommand(CMD_INFO);
    readBuffer(data);
    std::cout << (int)data[0] << "," << (int)data[1] << "," << (int)data[2] << std::endl;
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