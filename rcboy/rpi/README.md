# Raspberry PI

## Pinout

                           3V3     5V
                I2C SDA -- 2       5V
                I2C SCL -- 3      GND
                AVR_IRQ -- 4*      14 -- UART TX
                           GND     15 -- UART RX
             HEADPHONES -- 17      18 -- BTN_L
                  BTN_R -- 27*    GND
                  BTN_B -- 22*    *23 -- BTN_SELECT 
                           3V3    *24 -- BTN_A
    DISPLAY MOSI (SPI0) -- 10     GND
    DISPLAY MISO (SPI0) -- 9      *25 -- DISPLAY D/C
    DISPLAY SCLK (SPI0) -- 11       8 -- DISPLAY CE (SPI0 CE0)
                           GND      7 -- DISPLAY RESET (SPI0 CE1)
                  BTN_X -- 0        1 -- BTN_START
               NRF_RXTX -- 5*     GND
                NRF_IRQ -- 6*      12 -- AUDIO L
                AUDIO R -- 13     GND
              SPI1 MISO -- 19      16 -- SPI1 CE0
                  BTN_Y -- 26      20 -- SPI1 MOSI
                           GND     21 -- SPI1 SCLK
