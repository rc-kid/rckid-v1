
# Build

To launch a serial console, connect the serial to USB adapter to the TX RX and GND pins (for my setup it is white/green/black wires) and then launch the following command in terminal:

    minicom -b 115200 -o -D /dev/ttyUSB0

# Features

## Remote control

Control the attached peripherals. Can store multiple profiles and can in each profile map its controls to the controls of a given controller.

> I will be using mostly a Lego controller with servos, motors, RGB leds, buzzers and feedback buttons. A RGB strip/lights controller is also possible and finally a remote USB gamepad for computers should I ever need one. 

## Walkie-Talkie

The devices can use the onboard radio and act as a walkie-talkie, i.e. push button to talk & transmit, receive otherwise. 

## Game Console

A game console that can play some games. These can be either home made single player games, multiplayer games (pong), or I've heard there is a NES emulator for pico which can be used too.

> https://shop.pimoroni.com/products/picosystem - perhaps games for this can also be played? 

## MP3 player

Play mp3 files on the SD card. 

> If RPI won't do this then I can skip the I2S output and use a simple PWM as the sound quality will be pretty bad anyways.


# Parts

After a careful reconsideration, this is the actual list of parts I plan to use:

## RP2040

> https://rpishop.cz/waveshare/4461-921-waveshare-pico-like-mcu-deska-zalozena-na-raspberry-pi-mcu-rp2040-plus-verze.html#/152-varianta-bez_headeru

This is a simple low cost version (~200 CZK) that has all I need, i.e. lots of pins, li-po battery header, usb-c and 4MB flash. The flash might be a bit on a low side, if that would be the case I can go for the more expensive modules, namely pimoroni's Pic Li-Po, but thats 2x the price (https://botland.cz/desky-mikrokontroleru-rp2040/19764-pimoroni-pico-lipo-deska-s-mikrokontrolerem-rp2040-pimoroni-pim560-769894017258.html
).

## ATTiny1616

> I already have these from previous projects.

The ATtiny is always powered and monitors the battery level, charging, RTC, power button for RP2040 and non-volatile EEPROM memory (256bytes). It communicates with RP2040 via I2C and IRQ pin. It can also provide extra analog and digital inputs when needed. 

## Display

> https://www.laskakit.cz/176x220-barevny-lcd-tft-displej-2-0--spi/

A simple display that also gives a microSD card to be used as a disk for mp3s and assets. Uses SPI so only a few pins are necessary. The refresh rate via SPI should be enough for 30fps which is what I aim for.

## Controls

> analog 2 axis thumbstick: https://cz.mouser.com/ProductDetail/Adafruit/2765?qs=A50fv7uxK7XfNN3DwjrKxA%3D%3D
> I2C 3 axis gyro & accelerometer: https://www.gme.cz/modul-gyroskop-akcelerometr-i2c
> A, B, C, D buttons
> Select & start buttons (preferrably black, square and triangle caps)
> 2 shoulder buttons (maybe analog?)
> power button (connected to ATTiny, recessed)

## RF Modules

> https://www.gme.cz/modul-wifi-s-nrf24l01-pa-lna 
> https://www.gme.cz/modul-wifi-s-nrf24l01-integr-antena
> https://botland.cz/radiove-moduly/1795-radiovy-modul-rfm69hw-433s-433mhz-smd-transceiver-5904422300579.html
> LoRa 


# Parts

## RP2040

- some of the pico modules with integrated power management, i.e.

https://rpishop.cz/waveshare/4461-921-waveshare-pico-like-mcu-deska-zalozena-na-raspberry-pi-mcu-rp2040-plus-verze.html#/152-varianta-bez_headeru

> A simple version, 4MB flash, 26 pins, 3 ADCs, 1.8mA 3v3 source (I think it can't go this high really)


> twice the price, 4 times the flash (16MB).

## ATtiny1616

Have these already, can also be 3216 or similar. This is always on and turns the pico on/off. Directly connected to the power button. If there is not enough pins left on RP2040 after high speed peripherals, can be also used to handle the inputs as the pin count here will be rather small. 

## Display

2.8" 320x240 ili9341: https://www.laskakit.cz/2-8--palcovy-barevny-dotykovy-tft-lcd-displej-240x320-ili9341-spi/

> This is SPI only, maybe this will be too slow, but the layout is perfect.

https://rpishop.cz/waveshare/4002-waveshare-28-ips-lcd-displej-pro-raspberry-pi-pico-320240-spi.html

> An alternative, smaller SD card. Different driver & possibly faster parallel interface. The layout sucks though. 

## Joystick

https://cz.mouser.com/ProductDetail/Adafruit/2765?qs=A50fv7uxK7XfNN3DwjrKxA%3D%3D

## Accelerometer

https://www.gme.cz/modul-gyroskop-akcelerometr-i2c

## RF Module

https://www.gme.cz/modul-wifi-s-nrf24l01-pa-lna 
https://www.gme.cz/modul-wifi-s-nrf24l01-integr-antena
https://www.gme.cz/modul-wifi-s-nrf24l01-integr-antena

> The NRF has the fastest speed, but its range might be worse compared to the LoRa module. Altenatively both can be used and the user can switch? In that case perhaps use the smaller nrf as it will only be used to communicate with the modules I already have on short range. 

## Audio

- https://www.laskakit.cz/gy-pcm5102-i2s-audio-modul/
- https://www.pjrc.com/store/pt8211_kit.html

> pjrc is hard to get to europe, no good distributors. But I know it works decently, together with TPA311 at least. For microphone I can figure out the on the MEMS ones I have, or maybe PDM?

## Extras

- real time clock
- vibration motor
- buttons (maybe 3d printed caps)
- small and nice speaker
- i2c flash memory, or can use the memory on AVR to store data between on/off

# TODOs

- determine which display to use and how to connect to it
- determine which RF/antenna to use and what would the range be


5.5mm up
1.6mm pcb
1mm pico pcb
2.5mm USB-C
1
2.6
5.5
-------



