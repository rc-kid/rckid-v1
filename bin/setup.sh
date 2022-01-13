#!/bin/bash
sudo apt update
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential libstdc++-arm-none-eabi-newlib
#get the sdk
git clone -b master https://github.com/raspberrypi/pico-sdk.git sdk
cd sdk
git submodule update --init
cd ..

sudo groupadd --system usb
sudo usermod -a -G usb peta
sudo usermod -a -G tty peta
sudo usermod -a -G dialout peta

sudo apt-get install python3-pip
sudo pip3 install https://github.com/mraardvark/pyupdi/archive/master.zip


