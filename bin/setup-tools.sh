#!/bin/bash
# to enable serial monitors & consoles
sudo usermod -a -G dialout peta
sudo apt-get install screen minicom

# to enable SWD uploads to the pico from raspberry pi
sudo apt-get install automake autoconf build-essential texinfo libtool libftdi-dev libusb-1.0-0- dev

# to enable UF2 pyupdi for flashing AVR chips
sudo apt-get install python3-pip
sudo pip3 install https://github.com/mraardvark/pyupdi/archive/master.zip
