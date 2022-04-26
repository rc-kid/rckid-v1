#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>

#include "platform/platform.h"
#include "peripherals/nrf24l01.h"

/** Chip Pinout
               -- VDD             GND --
               -- (00) PA4   PA3 (16) -- SCK
               -- (01) PA5   PA2 (15) -- MISO
               -- (02) PA6   PA1 (14) -- MOSI
               -- (03) PA7   PA0 (17) -- UPDI
               -- (04) PB5   PC3 (13) -- NRF_CS
               -- (05) PB4   PC2 (12) -- NRF_RXTX
               -- (06) PB3   PC1 (11) -- NRF_IRQ
               -- (07) PB2   PC0 (10) -- 
           SDA -- (08) PB1   PB0 (09) -- SCL

    9 free pins

    2 = motor 1
    2 = motor 2
    1 = pwm
    1 = 
 */
constexpr gpio::Pin NRF_CS = 13;
constexpr gpio::Pin NRF_RXTX = 12;
constexpr gpio::Pin NRF_IRQ = 11;

NRF24L01 radio{NRF_CS, NRF_RXTX};
SSD1306AsciiWire oled;

// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C

// Define proper RST_PIN if required.
#define RST_PIN -1

void setup() {
    gpio::initialize();
    spi::initialize();
    i2c::initializeMaster();
  
    oled.begin(&Adafruit128x32, 0x3c);
    oled.setFont(Adafruit5x7);
    oled.clear();
    oled.println("Helo world!");    

    if (! radio.initialize("TEST2", "TEST1")) {
        //std::cout << "Failed to initialize NRF" << std::endl;
    }
    // TODO Set interrupt
    radio.standby();

}

void loop() {
}
