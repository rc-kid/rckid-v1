#!/bin/bash
echo "Syncing source code..."
#rsync -r -ssh avr-i2c-bootloader include bin CMakeLists.txt peta@10.0.0.38:/home/peta/devel/rcboy --delete
rsync -r -ssh avr-i2c-bootloader include bin CMakeLists.txt pi@10.0.0.39:/home/pi/rcboy --delete
echo "Buiding i2c-programmer"
#ssh peta@10.0.0.38 "mkdir -p ~/devel/rcboy/build-i2c-bootloader && cd ~/devel/rcboy/build-i2c-bootloader && cmake ../avr-i2c-bootloader -DARCH_RPI= -DCMAKE_BUILD_TYPE=Release && make"
ssh pi@10.0.0.39 "mkdir -p /home/pi/rcboy/build-i2c-bootloader && cd /home/pi/rcboy/build-i2c-bootloader && cmake ../avr-i2c-bootloader -DARCH_RPI= -DCMAKE_BUILD_TYPE=Release && make"