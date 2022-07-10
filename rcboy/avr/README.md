# ATTiny1616 Code

The ATTiny is always-on, responsible for analog user input, the volume buttons (which are also used to turn the device on) and some other inputs and outputs. 

> See the `src/avr.cpp` file for more details. 

## Overview

## Pinout

                   -- VDD             GND --
           AVR_IRQ -- (00) PA4   PA3 (16) -- VIB_EN
         BACKLIGHT -- (01) PA5   PA2 (15) -- CHARGE
            RGB_EN -- (02) PA6   PA1 (14) -- BTN_LVOL
             VBATT -- (03) PA7   PA0 (17) -- UPDI
          PHOTORES -- (04) PB5   PC3 (13) -- RGB
           MIC_OUT -- (05) PB4   PC2 (12) -- JOY_V
          BTN_RVOL -- (06) PB3   PC1 (11) -- JOY_BTN
            RPI_EN -- (07) PB2   PC0 (10) -- JOY_H
         SDA (I2C) -- (08) PB1   PB0 (09) -- SCL (I2C)



