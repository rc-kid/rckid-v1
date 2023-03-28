#!/bin/bash
echo "Syncing source code..."
rsync -r -ssh rckid bin tools include assets avr-i2c-bootloader CMakeLists.txt pi@10.0.0.39:/home/pi/rckid --delete
echo "Buiding rckid"
ssh pi@10.0.0.39 "mkdir -p ~/rckid/build && cd ~/rckid/build && cmake .. -DARCH_RPI= -DCMAKE_BUILD_TYPE=Release && make"