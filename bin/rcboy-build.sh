#!/bin/bash
echo "Syncing source code..."
rsync -r -ssh rcboy include assets avr-i2c-bootloader CMakeLists.txt pi@10.0.0.39:/home/pi/rcboy --delete
echo "Buiding rcboy"
ssh pi@10.0.0.39 "cd ~/rcboy/build && cmake .. -DARCH_RPI= -DCMAKE_BUILD_TYPE=Release && make"