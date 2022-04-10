#pragma once
#include <cstdint>
#include <pigpio.h>
#include <thread>
#include <chrono>

class cpu {
public:
    static void delay_us(unsigned value) {
        std::this_thread::sleep_for(std::chrono::microseconds(value));        
    }

    static void delay_ms(unsigned value) {
        std::this_thread::sleep_for(std::chrono::milliseconds(value));        
    }

    //static void sleep() {}

}; // cpu

//class wdt {
//public:
//    static void enable() {}
//    static void disable() {}
//    static void reset() {}
//}; // wdt

class gpio {
public:
    using Pin = unsigned;

    static void initialize() {
        gpioInitialise();
    }

    static void output(Pin pin) {
        gpioSetMode(pin, PI_OUTPUT);
    }

    static void input(Pin pin) {
        gpioSetMode(pin, PI_INPUT);
    }

    static void inputPullup(Pin pin) {
        gpioSetMode(pin, PI_OUTPUT);
        gpioSetPullUpDown(pin, PI_PUD_UP);
    }

    static void high(Pin pin) {
        gpioWrite(pin, 1);
    }

    static void low(Pin pin) {
        gpioWrite(pin, 0);
    }

    static bool read(Pin pin) { 
        return gpioRead(pin) == 1;
     }
}; // gpio

class i2c {
public:

    static void initializeMaster() {
        // TODO set speed here too
        // make sure that a write followed by a read to the same address will use repeated start as opposed to stop-start
        i2cSwitchCombined(true);
    }

    // static void initializeSlave(uint8_t address_) {}

    static bool transmit(uint8_t address, uint8_t const * wb, uint8_t wsize, uint8_t * rb, uint8_t rsize) {
        int h = i2cOpen(1, address, 0);
        if (h < 0)
            return false;
        if (wsize != 0)
            if (i2cWriteDevice(h, (char*)wb, wsize) != 0) {
                i2cClose(h);
                return false;
            }
        if (rsize != 0)
            if (i2cReadDevice(h, (char *)rb, rsize) != 0) {
                i2cClose(h);
                return false;
            }
        return i2cClose(h) == 0;
    }
}; // i2c

class spi {
public:

    /** Unlike most other implementations, device here is not a pin, but a device ID, which the auxiliary spi supports up to three. The number of devices supported and their pins are setup via overlays in `/boot/config.txt`, such as:
     
        dtoverlay=spi1-2cs,cs0_pin=16,cs1_pin=26
     */
    using Device = unsigned;

    /** No need for an initualization. 
     */
    static void initialize() {}

    /** Starts the transmission to given device. 
     */
    static void begin(Device device, unsigned baudrate) {
        handle_ = spiOpen(device, baudrate, SPI_AUX);
    }

    /** Terminates the SPI transmission and pulls the CE high. 
     */
    static void end(Device device) {
        spiClose(handle_);
    }

    /** Transfers a single byte.
     */
    static uint8_t transfer(uint8_t value) { 
        uint8_t result;
        spiXfer(handle_, & value, & result, 1);
    }

    static size_t transfer(uint8_t const * tx, uint8_t * rx, size_t numBytes) { 
        spiXfer(handle_, tx, rx, numBytes);
        return numBytes;
    }

    static void send(uint8_t const * data, size_t size) {
        spiWrite(handle_, static_cast<char*>(data), size);
    }

    static void receive(uint8_t * data, size_t size) {
        spiRead(handle_, static_cast<char*>(data), size);
    }

private:

    static constexpr unsigned SPI_AUX = 1 << 8;

    static inline int handle_ = PI_SPI_OPEN_FAILED;

    
}; // spi