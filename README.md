# RCkid

Handheld device mostly for kids that works as a retro games player, remote control for lego models and other stuff, walkie talkie, and a simple music/video player. Based on Rpi Zero 2W, ATTiny and NRF24L01P+ with a 320x240 SPI display, a selection of input controls (ABXY, DPAD, Thumbstick, Sel, Start, shoulder buttons, dedicated volume control buttons and a Home button), rumbler, light sensor, RGB torch and a custom software tailored towards the needs of the little ones. 

This is the main repository of the project containing all the necessary information to build the _RCkid_ itself and the remotely controlled devices.

## Overview

The _RCkid_ uses the RPi to run all the user interactive software, libretro via retropie to run emulated old school games and other software. The RPi is connected to digital buttons (ABXY, L, R, Volume) via gpio pins, to the display (`ILI9341`) with `SPI0` and to the NRF Radio via `SPI1`. `I2C` bus connects to the accelerometer, light sensor and the `ATTiny`. 

The `ATTiny` then manages the analog inputs (joystick, voltages, temperatures), multiplexed buttons (Dpad, Select & Start) and the Home/Power button. It is also responsible for the rumbler and the RGB LED as well as charging, battery management and a real-time clock. 

## Developing

> For building the RaspberryPi SD Card image see the [IMAGE.md](sd/IMAGE.md). This section deals with the active development setup. It has been tested on Windows 11, WSL Ubuntu 20. 

    git clone https://github.com/zduka/rckid-raylib.git
    cd rckid-raylib/src
    make PLATFORM=PLATFORM_DESKTOP
    cd ../..
    git clone https://github.com/zduka/rckid.git
    mkdir rckid/build
    cd rckid/build
    cmake ..

## Basic Operation

The `fbcp-ili9341` runs in its own process and is responsible for displaying the contents of the framebuffer on the SPI display.

> The display will freeze if someone changes the display properties. This seems to not happen if all apps run as `pi` (no root), but might be investigated further. 

The `rckid` app controls the UI, communicates with the AVR and creates and manages the virtual joystick and keyboard that is used to control the external applications. It uses a patched version of [raylib](https://github.com/zduka/rckid-raylib) that displays the UI at a level that will display itself over the 3rd party apps for videos and games. 

`vlc` and `retroarch` are used for playing videos and games respectively. They are launched by the ui and then controlled via the libevdev virtual gamepad and keyboard. 


### Prepare the RPi image

To build on Raspberry pi, create SD card image according to `sd/IMAGE.md`. This also installs all the prerequisites for building the rpi app and flashing the AVR. 

> As building on rpi is clumsy for development, most of rcboy can be developed on a normal computer (tested on Windows 11 WSL) and then uploaded to the rpi over WiFi. The repository contains tasks and build scripts necessary for such a setup using Visual Studio Code and can be easily adapted to different IDEs/editors.

### Flashing the AVR

When the RCBoy is assembled, the AVR chip is flashed via a custom I2C bootloader (the UPDI voltages would have to be translated otherwise, which is painful as the AVR runs on higher voltage than RPi). The RPi itself can be used to flash the AVR chip before soldered to the board by making a simple UPDI programmer from the serial port. After the bootloader is flashed, the RPi can be soldered to the board and the I2C programmer can be used for future updates. 


### Notes to Self 

- when stuff does not work, double and tripple check that all pins are properly soldered. Obviously no shorts, but weak bonds will result is horrible noise and/or non-functional elements
- a variant of the above, when soldering multiple PCBs together via holes, be extra sure that all PCBs actually have proper contact


# Notes


https://www.flaticon.com/

<div>Icons made by <a href="https://www.flaticon.com/authors/pixel-perfect" title="Pixel perfect">Pixel perfect</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/vectors-market" title="Vectors Market">Vectors Market</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/srip" title="srip">srip</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/monkik" title="monkik">monkik</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/aldo-cervantes" title="Aldo Cervantes">Aldo Cervantes</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.freepik.com" title="Freepik">Freepik</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/smashicons" title="Smashicons">Smashicons</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/umeicon" title="Umeicon">Umeicon</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/bukeicon" title="bukeicon">bukeicon</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/yogi-aprelliyanto" title="Yogi Aprelliyanto">Yogi Aprelliyanto</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/good-ware" title="Good Ware">Good Ware</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/apien" title="apien">apien</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/surang" title="surang">surang</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/chahir" title="chahir">chahir</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div>