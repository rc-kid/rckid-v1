#!/bin/bash
sudo apt update
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential libstdc++-arm-none-eabi-newlib
#get the sdk
git clone -b master https://github.com/raspberrypi/pico-sdk.git sdk
cd sdk
git submodule update --init
cd ..


