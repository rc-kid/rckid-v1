# RCKid

Handheld device mostly for kids that works as a retro games player, remote control for lego models and other stuff, walkie talkie, and a simple music/video player. Based on Rpi Zero 2W, ATTiny and NRF24L01P+ with a 320x240 SPI display, a selection of input controls (ABXY, DPAD, Thumbstick, Sel, Start, shoulder buttons, dedicated volume control buttons and a Home button), rumbler, light sensor, RGB torch and a custom software tailored towards the little ones. 

## BOM

Part                 | Quantity | Link 
---------------------|----------|------
Rpi Zero 2W          | 1        |
ATTiny1616           | 1        | (ATTiny3216 would work too)
ILI9341 320x240 SPI  | 1        |
Tactile Switch       | 9        |
NRF24L01P+           | 1        | Any module should work (*)


## PCB

Test Points:

- mic out
- speaker + and speaker - (can also be used to solder different speaker)
- pwm0, pwm1
- headphones
- btns1, btns2
- vrpi, 


## SD Card Preparation

## Assembly

> This repository contains all that is necessary to assemble and program your own _RCKid_. Please note that doing so is likely not a beginner project, at the very least, you will need access to a soldering station and some soldering practice. If there will be any errors during the assembly, multimeter, oscilloscope and some basic electronics knowledge would be required. Some software skills, such as SSH connection and flashing is necessary as well. 

First we are going to add the `ATTiny1616` and basic `I2C` communication to program the bootloader and verify its presence. To do so, the following must be soldered:

> TODO

When the bootloader is flashed, verify that the `I2C` communication is working by running the AVR programmer in query mode. 



# RCBoy

Handheld device for kids that provides the following:

- remote control for lego models & other stuff
- walkie talkie
- retro games player
- music / video player

Based on RPi Zero 2W, SPI display and nrf24l01p and/or sx1278 for comms

## Build

This section details the build & construction process of RCBoy. The process rather complex, but the steps here are in order that helps minimize problems:

- first the RPi is prepared with a proper SDCard image
- AVR and power management parts are soldered to the board
- the AVR bootloader is flashed via the RPi
- the rest of the board is soldered in particular order to allow accessibility
- rpi is soldered to the board
- the AVR application is programmed via I2C

### Prepare the RPi image

To build on Raspberry pi, create SD card image according to `sd/IMAGE.md`. This also installs all the prerequisites for building the rpi app and flashing the AVR. 

> As building on rpi is clumsy for development, most of rcboy can be developed on a normal computer (tested on Windows 11 WSL) and then uploaded to the rpi over WiFi. The repository contains tasks and build scripts necessary for such a setup using Visual Studio Code and can be easily adapted to different IDEs/editors.

### Flashing the AVR

When the RCBoy is assembled, the AVR chip is flashed via a custom I2C bootloader (the UPDI voltages would have to be translated otherwise, which is painful as the AVR runs on higher voltage than RPi). The RPi itself can be used to flash the AVR chip before soldered to the board by making a simple UPDI programmer from the serial port. After the bootloader is flashed, the RPi can be soldered to the board and the I2C programmer can be used for future updates. 

> TODO

### Adding contents

> TODO

## Hardware Assembly



### Notes to Self 

- when stuff does not work, double and tripple check that all pins are properly soldered. Obviously no shorts, but weak bonds will result is horrible noise and/or non-functional elements
- a variant of the above, when soldering multiple PCBs together via holes, be extra sure that all PCBs actually have proper contact

# Software

## Main Window

Runs on RPI, games, music, controllers, etc. 

## ATTiny

ATTiny is always on, but mostly sleeping. Talks to the RPI via I2C and 


# Build

To launch a serial console, connect the serial to USB adapter to the TX RX and GND pins (for my setup it is white/green/black wires) and then launch the following command in terminal:

    minicom -b 115200 -o -D /dev/ttyUSB0

https://www.flaticon.com/

<div>Icons made by <a href="https://www.flaticon.com/authors/pixel-perfect" title="Pixel perfect">Pixel perfect</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/vectors-market" title="Vectors Market">Vectors Market</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/srip" title="srip">srip</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/monkik" title="monkik">monkik</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/aldo-cervantes" title="Aldo Cervantes">Aldo Cervantes</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.freepik.com" title="Freepik">Freepik</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/smashicons" title="Smashicons">Smashicons</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/umeicon" title="Umeicon">Umeicon</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/bukeicon" title="bukeicon">bukeicon</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/yogi-aprelliyanto" title="Yogi Aprelliyanto">Yogi Aprelliyanto</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/good-ware" title="Good Ware">Good Ware</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/apien" title="apien">apien</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/surang" title="surang">surang</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/chahir" title="chahir">chahir</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div>