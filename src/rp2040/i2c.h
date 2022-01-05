#pragma once

#include <hardware/i2c.h>
#include <pico/binary_info.h>


namespace i2c {

    inline void initialize(i2c_inst_t * controller, unsigned sda, unsigned scl, unsigned baudrate = 400000) {
        i2c_init(controller, 400 * 1000);
        gpio_set_function(sda, GPIO_FUNC_I2C);
        gpio_set_function(scl, GPIO_FUNC_I2C);
        // Make the I2C pins available to picotool
        bi_decl(bi_2pins_with_func(sda, scl, GPIO_FUNC_I2C));    
    }

    inline void initialize(unsigned sda, unsigned scl, unsigned baudrate = 400000) {
        initialize(i2c0, sda, scl, baudrate);
    }


    /** A simple class that unifies the interface of I2C devices.
     */
    class Device {
    public:

        uint8_t const address;

    protected:

        Device(uint8_t address):
            address{address},
            controller_{i2c0} {
        }

        Device(uint8_t address, i2c_inst_t * controller):
            address{address},
            controller_{controller} {
        }

        void readRegister(uint8_t reg, uint8_t * buffer, uint8_t size) {
            i2c_write_blocking(controller_, address, & reg, 1, true);
            i2c_write_blocking(controller_, address, buffer, size, false);
        }

        uint8_t readRegister8(uint8_t reg) {
            i2c_write_blocking(controller_, address, & reg, 1, true);
            uint8_t result;
            i2c_read_blocking(controller_, address, & result, 1, false);
            return result;
        }

        uint16_t readRegister16(uint8_t reg) {
            i2c_write_blocking(controller_, address, & reg, 1, true);
            uint16_t result;
            i2c_read_blocking(controller_, address, reinterpret_cast<uint8_t*>(& result), 2, false);
            return result;
        }

        void writeRegister8(uint8_t reg, uint8_t value) {
            uint8_t buf[] = { reg, value };
            i2c_write_blocking(controller_, address, buf, sizeof(buf), false);
        }

        void writeRegister16(uint8_t reg, uint16_t value) {

        }

    private:

        i2c_inst_t * controller_;
        

    }; // Device

}

