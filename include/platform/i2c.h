#pragma once
#include "platform.h"

#if (defined ARCH_RP2040)
#include <hardware/i2c.h>
#elif (defined ARCH_AVR_MEGATINY)
#elif (defined ARCH_ARDUINO)
#include <Wire.h>
#else
ARCH_NOT_SUPPORTED;
#endif

namespace i2c {

#if (defined ARCH_RP2040)
    inline void initialize(unsigned sda, unsigned scl, unsigned baudrate = 400000) {
        i2c_init(i2c1, 400 * 1000);
        gpio_set_function(sda, GPIO_FUNC_I2C);
        gpio_set_function(scl, GPIO_FUNC_I2C);
        // Make the I2C pins available to picotool
        bi_decl(bi_2pins_with_func(sda, scl, GPIO_FUNC_I2C));    
    }
#else

    inline void initialize() {
#if (defined __AVR_ATmega8__)
        TWBR = static_cast<uint8_t>((F_CPU / 100000 - 16) / 2);
        TWAR = 0; // master mode
        TWDR = 0xFF; // default content = SDA released.
        TWCR = Bits<TWEN,TWIE>::value();         
#elif (defined ARCH_AVR_MEGATINY)
        // TODO TODO
#elif (defined ARCH_ARDUINO)
        Wire.begin();
        Wire.setClock(400000);
#endif
    }
#endif

#if (defined ARCH_AVR_MEGATINY)
    /** I2C Slave initialization for Series 1 chips. 
     */
    inline void initializeSlave(uint8_t address) {
        cli();
        // turn I2C off in case it was running before
        TWI0.MCTRLA = 0;
        TWI0.SCTRLA = 0;
        // make sure that the pins are nout out - HW issue with the chip, will fail otherwise
        PORTB.OUTCLR = 0x03; // PB0, PB1
        // set the address and disable general call, disable second address and set no address mask (i.e. only the actual address will be responded to)
        TWI0.SADDR = address << 1;
        TWI0.SADDRMASK = 0;
        // enable the TWI in slave mode, enable all interrupts
        TWI0.SCTRLA = TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm  | TWI_ENABLE_bm;
        // bus Error Detection circuitry needs Master enabled to work 
        // not sure why we need it
        TWI0.MCTRLA = TWI_ENABLE_bm;   
        sei();
    }
#endif


    inline bool transmit(uint8_t address, uint8_t * wb, uint8_t wsize, uint8_t * rb, uint8_t rsize) {
#if (defined ARCH_RP2040)
        if (wsize != 0)
            i2c_write_blocking(i2c1, address, wb, wsize, rsize != 0);
        if (rsize != 0)
            i2c_read_blocking(i2c1, address, rb, rsize, false);
        return true;
#elif (defined __AVR_ATmega8__)
       // TODO won't compile for now
       // update the slave address by shifting it to the right and then adding 1 if we are going to read immediately, i.e. no transmit bytes
        address <<= 1;
        if (txSize == 0)
            address += 1;
    i2c_master_start:
        // send the start condition
        TWCR = Bits<TWEN, TWINT, TWSTA>::value();
        switch (Wait()) {
            case START:
            case REPEATED_START:
                break;
            default:
                goto i2c_master_error;
        }
        TWDR = address;
        TWCR = Bits<TWEN,TWINT>::value();
        // determine whether we are sending or receiving 
        switch (Wait()) {
            case SLAVE_WRITE_ACK: {
                // if slave write has been acknowledged, transmit the bytes we have to send
                uint8_t const * txBytes = static_cast<uint8_t const *>(tx);
                while (txSize > 0) {
                    TWDR = *txBytes;
                    ++txBytes;
                    --txSize;
                    TWCR = Bits<TWEN,TWINT>::value();
                    if (Wait() != SLAVE_DATA_WRITE_ACK)
                        goto i2c_master_error;
                }
                // done, send the stop condition if there are no bytes to be received
                if (rxSize == 0)
                    break;
                // otherwise update slave address to indicate reading and send repeated start
                slave += 1;
                goto i2c_master_start;
            }
            case SLAVE_READ_ACK: {
                uint8_t * rxBytes = static_cast<uint8_t *>(rx);
                while (rxSize > 0) {
                    if (rxSize == 1) 
                        TWCR = Bits<TWEN, TWINT>::value();
                    else
                        TWCR = Bits<TWEN, TWINT, TWEA>::value();
                    switch (Wait()) {
                        case SLAVE_DATA_READ_ACK:
                        case SLAVE_DATA_READ_NACK:
                            break;
                    default:
                        goto i2c_master_error;
                    }
                    *rxBytes = TWDR;
                    ++rxBytes;
                    --rxSize;
                }
                break;
            }
            default:
                goto i2c_master_error;
        }
        // transmit the stop condition and exit successfully
        TWCR = Bits<TWINT,TWEN,TWSTO>::value();
        return true;
        // an error
    i2c_master_error:
        TWCR = Bits<TWINT,TWEN,TWSTO>::value();
        return false;
#elif (defined ARCH_AVR_MEGATINY)
        // TODO TODO
#elif (defined ARCH_ARDUINO)
        if (wsize > 0) {
            Wire.beginTransmission(address);
            Wire.write(wb, wsize);
            Wire.endTransmission(rsize == 0); 
        }
        if (rsize > 0) {
            if (Wire.requestFrom(address, rsize) == rsize) {
                Wire.readBytes(rb, rsize);
            } else {
                Wire.flush();
                return false;
            }
        }
        return true;
#endif
    }

    /** A prototype of I2C device. 
     */
    class Device {
    public:
        const uint8_t address;
    protected:
        Device(uint8_t address):
            address{address} {
        }

        bool isPresent() {
            return transmit(address, nullptr, 0, nullptr, 0);
        }

        template<typename T>
        void write(T data);

        void write(uint8_t * data, uint8_t size) {
            transmit(address, data, size, nullptr, 0);
        }

        template<typename T>
        T read(); 

        uint8_t read(uint8_t * buffer, uint8_t size) {
            return transmit(address, buffer, size, nullptr, 0) ? size : 0;
        }

        template<typename T>
        void writeRegister(uint8_t reg, T value);

        template<typename T>
        T readRegister(uint8_t reg);

    }; // i2c::Device

    template<>
    void Device::write<uint8_t>(uint8_t data) {
        transmit(address, & data, 1, nullptr, 0);
    }

    template<>
    void Device::write<uint16_t>(uint16_t data) {
        transmit(address, reinterpret_cast<uint8_t *>(& data), 2, nullptr, 0);
    }   

    template<>
    uint8_t Device::read<uint8_t>() {
        uint8_t result = 0;
        transmit(address, nullptr, 0, & result, 1);
        return result;
    }

    template<>
    uint16_t Device::read<uint16_t>() {
        uint16_t result = 0;
        transmit(address, nullptr, 0, reinterpret_cast<uint8_t *>(result), 2);
        return result;
    }

    template<>
    void Device::writeRegister<uint8_t>(uint8_t reg, uint8_t value) {
        uint8_t buf[] = { reg, value };
        transmit(address, buf, 2, nullptr, 0);
    }

    /*
    template<>
    void Device::writeRegister<uint16_t>(uint8_t reg, uint16_t value) {
    }
    */

    template<>
    uint8_t Device::readRegister<uint8_t>(uint8_t reg) {
        uint8_t result = 0;
        transmit(address, & reg, 1, & result, 1);
        return result;
    }

    template<>
    uint16_t Device::readRegister<uint16_t>(uint8_t reg) {
        uint16_t result = 0;
        transmit(address, & reg, 1, reinterpret_cast<uint8_t*>(& result), 2);
        return result;
    }

} // namespace i2c