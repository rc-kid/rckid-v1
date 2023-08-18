#!/bin/bash
echo "Syncing source code..."
rsync -r -ssh rckid include assets avr-i2c-bootloader CMakeLists.txt pi@10.0.0.40:/home/pi/rckid --delete
echo "Buiding rckid"
ssh pi@10.0.0.40 "cd ~/rckid/build && cmake .. -DARCH_RPI= -DCMAKE_BUILD_TYPE=Release && make i2c-programmer"
