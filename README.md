# RCBoy

Handheld device for kids that provides the following:

- remote control for lego models & other stuff
- walkie talkie
- retro games player
- music / video player

Based on RPi Zero 2W, SPI display and nrf24l01p and/or sx1278 for comms

# Software

## Main GUI

Runs on RPI, games, music, controllers, etc. 

## ATTiny

ATTiny is always on, but mostly sleeping. Talks to the RPI via I2C and 


# SD Card Image

Install the development dependencies:

    sudo apt-get install libevdev-dev
    sudo apt-get install pigpio

Add the udev rule for the gamepad to be recognized as a joystick so that emulation station and retroarch pick it up by copying the `rpi/gamepad/99-rcboy-gamepad.rules` file to `/etc/udev/rules/d`. 

> TODO verify with new image whether the libi2c-dev is needed when using pygpio

Install platformio:

    python3 -c "$(curl -fsSL https://raw.githubusercontent.com/platformio/platformio/master/scripts/get-platformio.py)"


Add the `rpi/ili9341/ili9341.service` to `/lib/systemd/system` and run:

    sudo systemctl enable ili9341


# BOM

RPI Zero 2 W    | 1   |  4  |
3.2" Display    | 1   |  3  | https://www.aliexpress.com/item/32861524235.html?gatewayAdapt=glo2isr&spm=a2g0o.order_list.0.0.21ef586aQegknE
AtTiny3216      | 1   |     |
Push-button     | 6   | 30  |
2-Axis Joystick | 1   |     | https://www.aliexpress.com/item/33051414032.html?spm=a2g0o.order_list.0.0.21ef1802WC1CIu
TPA311          | 1   |     |
3V3 SW Reg      | 1   |     |
USB-C Charger   | 1   | 10  | https://hadex.cz/m401e-nabijecka-li-ion-clanku-1a-s-ochranou-modul-s-io-tp4056-usb-c/
NRF24L01P       | 1   |     |
SX1278          | 1   |     |
Speaker         | 1   |     | https://www.tme.eu/cz/details/ld-sp-u15.5_8a/reproduktory/loudity/
3000mAh battery | 1   |     | https://www.laskakit.cz/baterie-li-po-3-7v-3000mah-lipo/

# Build

To launch a serial console, connect the serial to USB adapter to the TX RX and GND pins (for my setup it is white/green/black wires) and then launch the following command in terminal:

    minicom -b 115200 -o -D /dev/ttyUSB0

## Extras

- real time clock
- vibration motor
- buttons (maybe 3d printed caps)
- small and nice speaker
- i2c flash memory, or can use the memory on AVR to store data between on/off
