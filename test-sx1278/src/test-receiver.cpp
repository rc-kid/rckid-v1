#include <Arduino.h>

#include "LoRa.h"

#include "peripherals/pcd8544.h"
#include "peripherals/nrf24l01.h"

/** Chip Pinout
               -- VDD             GND --
               -- (00) PA4   PA3 (16) -- SCK
               -- (01) PA5   PA2 (15) -- MISO
               -- (02) PA6   PA1 (14) -- MOSI
               -- (03) PA7   PA0 (17) -- UPDI
               -- (04) PB5   PC3 (13) -- 
               -- (05) PB4   PC2 (12) -- 
               -- (06) PB3   PC1 (11) -- 
               -- (07) PB2   PC0 (10) -- DISPLAY_CE
   DISPLAY_RST -- (08) PB1   PB0 (09) -- DISPLAY_DC
 */

static constexpr gpio::Pin DISPLAY_CE = 10;
static constexpr gpio::Pin DISPLAY_DC = 9;
static constexpr gpio::Pin DISPLAY_RST = 8;

PCD8544<DISPLAY_RST, DISPLAY_CE, DISPLAY_DC> display_;



void setup() {
    spi::initialize();
    display_.enable();
    display_.clear();
    cpu::delay_ms(100);
    display_.write(0,0,"RECV SX1278");
}

void loop() {
}
