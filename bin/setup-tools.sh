#!/bin/bash
# to enable serial monitors & consoles
sudo usermod -a -G dialout peta
sudo apt-get install screen minicom

# to enable SWD uploads to the pico from raspberry pi
# from https://www.kotakenterprise.com/2021/03/26/how-to-program-and-debug-raspberry-pi-pico-with-swd/#:~:text=Raspberry%20Pi%20Pico%20SWD%20Programming%20and%20Debug%20Like,SWDIO%20%28bidirectional%20SWD%20Data%29%20and%20SWCLK%20%28SWD%20Clock%29.
sudo apt-get install automake autoconf build-essential texinfo libtool libftdi-dev libusb-1.0-0- dev
git clone https://github.com/raspberrypi/openocd.git --recursive --branch rp2040 --depth=1
cd openocd
./configure --enable-ftdi --enable-sysfsgpio --enable-bcm2835gpio
cd ..


# to enable UF2 pyupdi for flashing AVR chips
sudo apt-get install python3-pip
sudo pip3 install https://github.com/mraardvark/pyupdi/archive/master.zip
