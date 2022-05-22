#!/bin/bash
echo "Syncing source code..."
rsync -r -ssh rpi include common pi@10.0.0.7:/home/pi/rcboy
echo "Buiding rcboy"
ssh pi@10.0.0.7 "cd ~/rcboy/build && cmake .. -DARCH_RPI= && make -j"