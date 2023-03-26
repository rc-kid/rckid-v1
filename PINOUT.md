# Raspberry PI Pinout

                           3V3     5V
                I2C SDA -- 2       5V
                I2C SCL -- 3      GND
                AVR_IRQ -- 4*      14 -- UART TX
                           GND     15 -- UART RX
                      A -- 17      18 -- B
                      X -- 27*    GND
                      Y -- 22*    *23 -- L 
                           3V3    *24 -- R
    DISPLAY MOSI (SPI0) -- 10     GND
    DISPLAY MISO (SPI0) -- 9      *25 -- DISPLAY D/C
    DISPLAY SCLK (SPI0) -- 11       8 -- DISPLAY CE (SPI0 CE0)
                           GND      7 -- DISPLAY RESET (SPI0 CE1)
                    ??? -- 0        1 -- ???
               NRF_RXTX -- 5*     GND
                NRF_IRQ -- 6*      12 -- AUDIO L
                AUDIO R -- 13     GND
              SPI1 MISO -- 19      16 -- SPI1 CE0
               SPI1 CE1 -- 26      20 -- SPI1 MOSI
                           GND     21 -- SPI1 SCLK

> 28 available
> (6) SPI display
> (3) SPI1
> (3) CS, CSN, IRQ for NRF24l01p
> (3) CS, D1, D2 for SX1278
> (2) I2C
> (1) AVR IRQ
> (2) AUDIO PWM

> 8

Wired display pinout:

red = 3V3
pink = MOSI
yellow = MISO ???????
green = SCLK
blue = D/C
white = CE
  = RESET?
brown = to gnd with 10R  



# AVR Pinout


                 VDD   GND
                 PA4   PA3
                 PA5   PA2
                 PA6   PA1
                 PA7   PA0 -- UPDI
                 PB5   PC3
                 PB4   PC2
                 PB3   PC1
                 PB2   PC0
      I2C SDA -- PB1   PB0 -- I2C SCL

> 17 available
> (2) I2C
> (1) AVR IRQ
> (8) A, B, X, Y, L, R, SEL, START 
> (2) JOY_X, JOY_Y
> (2) for charging, V_BATT

> (2)

- must do display brightness as the available PWM channels are occupied by audio out (and we can't output I2S as that conflicts with SPI1)
- arguably also the motor
- some of the simple buttons can be switched to RPI