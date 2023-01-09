#include <iostream>
#include <iomanip>

#include "platform/platform.h"

#include "config.h"

void sendCommand(uint8_t cmd) {
    if (! i2c::transmit(I2C_ADDRESS, & cmd, 1, nullptr, 0))
        throw STR("Cannot send command " << (int)cmd);
    cpu::delay_ms(10);
}

void sendCommand(uint8_t cmd, uint8_t arg) {
    uint8_t data[] = { cmd, arg};
    if (! i2c::transmit(I2C_ADDRESS, data, 2, nullptr, 0))
        throw STR("Cannot send command " << (int)cmd << ", arg " << (int)arg);
    cpu::delay_ms(10);
}

void readBuffer(uint8_t * buffer) {
    if (! i2c::transmit(I2C_ADDRESS, nullptr, 0, buffer, 32))
        {} // throw STR("Cannot read bootloader's buffer");
}

void writeBuffer(uint8_t const * buffer) {
    uint8_t data[33];
    data[0] = CMD_WRITE_BUFFER;
    for (size_t i = 0; i < 32; ++i)
        data[i+1] = buffer[i];
    if (! i2c::transmit(I2C_ADDRESS, data, 33, nullptr, 0))
        throw STR("Cannot write bootloader's buffer");
}

void fusesTinySeries1(uint8_t * data, char const * chipName) {
    std::cout << "ATTiny Series 1 detected:" << std::endl;
    std::cout << "    chip:               " << std::hex << (unsigned)data[0] << ":" << (unsigned)data[1] << ":" << (unsigned)data[2] << std::endl;
    std::cout << "    FUSE.WDTCFG:        " << (unsigned(data[3])) << std::endl;
    std::cout << "    FUSE.BODCFG:        " << (unsigned(data[4])) << std::endl;
    std::cout << "    FUSE.OSCCFG:        " << (unsigned(data[5])) << std::endl;
    std::cout << "    FUSE.TCD0CFG:       " << (unsigned(data[6])) << std::endl;
    std::cout << "    FUSE.SYSCFG0:       " << (unsigned(data[7])) << std::endl;
    std::cout << "    FUSE.SYSCFG1:       " << (unsigned(data[8])) << std::endl;
    std::cout << "    FUSE.APPEND:        " << (unsigned(data[9])) << std::endl;
    std::cout << "    FUSE.BOOTEND:       " << (unsigned(data[10])) << std::endl;
    std::cout << "    CLKCTRL.MCLKCTRLA:  " << (unsigned(data[11])) << std::endl;
    std::cout << "    CLKCTRL.MCLKCTRLB:  " << (unsigned(data[12])) << std::endl;
    std::cout << "    CLKCTRL.MCLKLOCK:   " << (unsigned(data[13])) << std::endl;
    std::cout << "    CLKCTRL.MCLKSTATUS: " << (unsigned(data[14])) << std::endl;


}

void checkAVRChip(uint8_t * data) {
    uint32_t signature = (data[0] << 16) + (data[1] << 8) + data[2];
    switch (signature) {
        case 0x1e9422: 
            fusesTinySeries1(data, "ATTiny1614");
            break;
        case 0x1e9421:
            fusesTinySeries1(data, "ATTiny1616");
            break;
        case 0x1e9420:
            fusesTinySeries1(data, "ATTiny1617");
            break;
        default:
            throw STR("Unknown chip detected, signature: " << std::hex << (unsigned)data[0] << ":" << (unsigned)data[1] << ":" << (unsigned)data[2]);
    }
}


void enterBootloader() {
    // check that the avr is present
    if (!i2c::transmit(I2C_ADDRESS, nullptr, 0, nullptr, 0))
        throw STR("Device not detected at I2C address " << I2C_ADDRESS);
    cpu::delay_ms(10);
    // to enter the bootloader, pull the AVR_IRQ pin low first
    // gpio::output()
    //
    // reset the AVR
    sendCommand(CMD_RESET);
    // verify that we have proper communications available by writing some stuff and then reading it back
    sendCommand(CMD_CLEAR_INDEX);
    char const * text = "ThisIsATest_DoYouHearMe?12345678\0";
    writeBuffer((uint8_t const *)text);
    sendCommand(CMD_CLEAR_INDEX);
    uint8_t data[33];
    readBuffer(data);
    if (strncmp(text, (char const *)data, 32) != 0) {
        data[32] = 0;
        throw STR("IO Error. Device detected but I2C communication not working:\n   Sent:     " << text << "\n" << "   Received: " << data);
    }
    sendCommand(CMD_INFO);
    sendCommand(CMD_CLEAR_INDEX);
    readBuffer(data);
    checkAVRChip(data);
    sendCommand(CMD_READ_PAGE, 0);
    sendCommand(CMD_CLEAR_INDEX);
    readBuffer(data);
    for (int i = 0; i < 32; ++i)
        std::cout << std::hex << (unsigned)data[i];
    std::cout << std::endl;
}

int main(int argc, char * argv[]) {
    // initialize gpio and i2c
    gpio::initialize();
    i2c::initializeMaster();
    try {
        // check if we have AVR ready
        enterBootloader();
        return EXIT_SUCCESS;
    } catch (std::string const & e) {
        std::cerr << e << std::endl;
        return EXIT_FAILURE;
    }
}