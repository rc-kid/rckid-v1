#pragma once
#include <cstdint>
#include <cstring>
#include <pigpio.h>
#include <thread>
#include <chrono>

#include "spinlock.h"

#define STR(...) static_cast<std::stringstream &&>(std::stringstream() << __VA_ARGS__).str()

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
    static constexpr Pin UNUSED = 0xffffffff;

    static void initialize() {
        gpioInitialise();
    }

    static void output(Pin pin) {
        gpioSetMode(pin, PI_OUTPUT);
    }

    static void input(Pin pin) {
        gpioSetMode(pin, PI_INPUT);
        gpioSetPullUpDown(pin, PI_PUD_OFF);
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


/** The default spi implementation via pigpio sets the CS line before and after each transfer which does not play nice with the smaller MCUs such as AVR where multiple sequential transfers are used for a single transaction. Fortunately, the CS pins can be ignored by the pigpio, so they can be controlled by the user, which is what we do here. 
 */
class spi {
public:

    using Device = gpio::Pin;

    static void initialize(unsigned baudrate = 5000000) {
        baudrate_ = baudrate;
    }

    /** Starts the transmission to given device. 
     */
    static void begin(Device device) {
        handle_ = spiOpen(0, baudrate_, SPI_AUX | SPI_RES_CE0 | SPI_RES_CE1 | SPI_RES_CE2);
        gpio::low(device);
    }

    /** Terminates the SPI transmission and pulls the CE high. 
     */
    static void end(Device device) {
        spiClose(handle_);
        gpio::high(device);
    }

    /** Transfers a single byte.
     */
    static uint8_t transfer(uint8_t value) { 
        uint8_t result;
        spiXfer(handle_, reinterpret_cast<char*>(& value), reinterpret_cast<char*>(& result), 1);
        return result;
    }

    static size_t transfer(uint8_t const * tx, uint8_t * rx, size_t numBytes) { 
        spiXfer(handle_, reinterpret_cast<char*>(const_cast<uint8_t*>(tx)), reinterpret_cast<char*>(rx), numBytes);
        return numBytes;
    }

    static void send(uint8_t const * data, size_t size) {
        spiWrite(handle_, reinterpret_cast<char*>(const_cast<uint8_t*>(data)), size);
    }

    static void receive(uint8_t * data, size_t size) {
        spiRead(handle_, reinterpret_cast<char*>(data), size);
    }

private:

    static constexpr unsigned SPI_AUX = 1 << 8;
    static constexpr unsigned SPI_RES_CE0 = 1 << 5;
    static constexpr unsigned SPI_RES_CE1 = 1 << 6;
    static constexpr unsigned SPI_RES_CE2 = 1 << 7;

    static inline unsigned baudrate_;

    static inline int handle_ = PI_SPI_OPEN_FAILED;

    
}; // spi
