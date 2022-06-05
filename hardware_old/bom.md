# Overview

## RP2040

Pins available:
- 3 analog
- 26 digital (including the analog pins)

RP2040 must handle the following:
- 12 pins for the display (8 + 4). Arguably only 8+3 is required if no tear line, but that might be pushing it
- 3 pins for I2S output
- 2 pins for microphone
- 3 pins for SPI (MISO, MOSI, SCK)
- 2 pins for I2C
- 1 pin for AVR IRQ
- 3 pins for NRF24

3 left


- 2 pins for ATTiny (IRQ, CS)
- 3 pins for NRF (IRQ, CS, RXTX)
- 2 

# ATTiny3216

17 pins. The analog & digital stuff

- 2 analog for joystick
- 7 digital for buttons (A, B, C, D, L, R, Power)
- 2 for I2C
- 1 for IRQ
- 1 for RP2040 enable

4 left


- 8 + 4 for the display 
- 3 for spi
- 2 for i2c
- 3 for i2s
- 3 for NRF
- 1 for AVR IRQ
- 2 for RFM69


## Display

320x240px, 2.8" display, ILI9341 controller, parallel 8bit interface. 

> After some deliberation and trial and error, this looks like a promising display: https://www.aliexpress.com/item/32837085500.html?spm=a2g0o.cart.0.0.75c93c00EkW06s&mp=1, but since the cable is 0.5mm pitch, here is also an adapter. The cable seems to have multiple connection options as well as the tearing feedback.