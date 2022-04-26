#pragma once
#error "Not finished"
#include "platform/platform.h"

/** The SSD1306 OLED display controller. 
 
    Supports monochrome OLED displays of 128x32 and 128x64 pixels connected via I2C bus. A fresh reimplementation for minimal footprint. 
 */
class SSD1306 : public I2CDevice {
public:

    SSD1306(uint8_t address):
        I2CDevice{address} {
    }

    void initialize128x32() {
        uint8_t cmd[] = {
            COMMAND_MODE,
            DISPLAY_OFF, // turn off
            0xd5, 0x80, // set display clock divide/osc frequency to 128, which should be there after reset already
            0xa8, 0x1f, // set multiplex ratio to 31
            0xd3, 0x00, // set display offset to 0
            0x40, // set display start line to 0
            0x8d, 0x14, //???
            0xa1, // set segment remap, address 127 mapped to segment 0
            0xc8, // remapped mode, from COM n-1 to COM 0
            0xda,0x02, // set COM pins HW config to normal
            SET_CONTRAST, 0x7f, // set contrast to 127
            0xd9,0xf1, // set pre-charge period to 111 1001, 0.83 VCC, ???
            0xdb,0x40, // set deselect level to ???
            DISPLAY_RAM, // turn display to use what's in RAM
            NORMAL_MODE, // set display normal mode
            DISPLAY_ON // turn display on
        };
        I2CDevice::write(cmd, sizeof(cmd));
    }

    void normalMode() {
        uint8_t cmd[] = { COMMAND_MODE, NORMAL_MODE };
        I2CDevice::write(cmd, sizeof(cmd));
    }

    void inverseMode() {
        uint8_t cmd[] = { COMMAND_MODE, INVERSE_MODE };
        I2CDevice::write(cmd, sizeof(cmd));
    }

    void setContrast(uint8_t value) {
        uint8_t cmd[] = { SET_CONTRAST, value };
        I2CDevice::write(cmd, sizeof(cmd));
    }

    void clear32() {
        /*
        i2c::startWrite(address);
        i2c::write(COMMAND_MODE);
        i2c::write(SET_PAGE);
        i2c::write(SET_COLUMN_LOW);
        i2c::writeRestart(SET_COLUMN_HIGH);
        i2c::startWrite(address);
        i2c::write(DATA_MODE);
        for (uint16_t i = 0; i < (128 * 32 / 8) - 1; ++i)
            i2c::write(0);
        i2c::writeLast(0);
        */
    }

    void clear64() {
        /*
        i2c::startWrite(address);
        i2c::write(COMMAND_MODE);
        i2c::write(SET_PAGE);
        i2c::write(SET_COLUMN_LOW);
        i2c::writeRestart(SET_COLUMN_HIGH);
        i2c::startWrite(address);
        i2c::write(DATA_MODE);
        for (uint16_t i = 0; i < (128 * 64 / 8) - 1; ++i)
            i2c::write(0);
        i2c::writeLast(0);
        */
    }

    void gotoXY(uint8_t col, uint8_t row) {
        uint8_t cmd[] = { address, COMMAND_MODE, SET_PAGE | row, SET_COLUMN_LOW | (col & 0xf), SET_COLUMN_HIGH | ((col >> 4) & 0xf)};
        I2CDevice::write(cmd, sizeof (cmd));
        /*
        i2c::startWrite(address);
        i2c::write(COMMAND_MODE);
        i2c::write(SET_PAGE | row);
        i2c::write(SET_COLUMN_LOW | (col & 0xf));
        i2c::writeLast(SET_COLUMN_HIGH | ((col >> 4) & 0xf));
        */
    }

    void write(char x) {

    }

    void writeData(uint8_t col, uint8_t row, uint8_t * data, uint16_t size) {
    }

private:

    static constexpr uint8_t COMMAND_MODE  = 0x00;
    static constexpr uint8_t DATA_MODE = 0x40;
    static constexpr uint8_t SET_COLUMN_LOW = 0x00;
    static constexpr uint8_t SET_COLUMN_HIGH = 0x10; 
    static constexpr uint8_t SET_PAGE = 0xb0;

    static constexpr uint8_t DISPLAY_OFF = 0xae;
    static constexpr uint8_t DISPLAY_ON = 0xaf;
    static constexpr uint8_t NORMAL_MODE = 0xa6;
    static constexpr uint8_t INVERSE_MODE = 0xa7;
    static constexpr uint8_t DISPLAY_ALL = 0xa5;
    static constexpr uint8_t DISPLAY_RAM = 0xa4;
    static constexpr uint8_t SET_CONTRAST = 0x81;

/*
    static constexpr PROGMEM uint8_t init32_[] = {
        COMMAND_MODE,
        DISPLAY_OFF, // turn off
        0xd5, 0x80, // set display clock divide/osc frequency to 128, which should be there after reset already
        0xa8, 0x1f, // set multiplex ratio to 31
        0xd3, 0x00, // set display offset to 0
        0x40, // set display start line to 0
        0x8d, 0x14, //???
        0xa1, // set segment remap, address 127 mapped to segment 0
        0xc8, // remapped mode, from COM n-1 to COM 0
        0xda,0x02, // set COM pins HW config to normal
        SET_CONTRAST, 0x7f, // set contrast to 127
        0xd9,0xf1, // set pre-charge period to 111 1001, 0.83 VCC, ???
        0xdb,0x40, // set deselect level to ???
        DISPLAY_RAM, // turn display to use what's in RAM
        NORMAL_MODE, // set display normal mode
        DISPLAY_ON // turn display on
    };


    static constexpr PROGMEM uint8_t init64_[] = {
        0x00, // next command
        0xae, // turn off
        0xa8, 0x3f, // set multiplex ratio to 63
        0xd3, 0x00, // set display offset to 0
        0x40, // set display start line to 0
        0xa1, // set segment remap, address 127 mapped to segment 0
        0xc8, // remapped mode, from COM n-1 to COM 0
        0xda,0x12, // set COM pins HW config to alternative
        0x81,0xff, // set contrast to max
        0xa4, // turn display to use what's in RAM
        0xa6, // set display normal mode
        0xd5,0x80, // set display clock divide/osc frequency to 128, which should be there after reset already
        0x8d,0x14, // ???
        0xaf, // turn display on
        0x20, 0x02 // set memory addressing mode to page addressing mode (RESET)
    };
 */
    


}; // SSD1306