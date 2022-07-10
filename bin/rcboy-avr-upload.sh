#!/bin/bash
echo "Syncing source code..."
rsync -r -ssh rcboy/avr include common pi@10.0.0.39:/home/pi/rcboy/rcboy
echo "Buiding avr-src"
ssh pi@10.0.0.39 "cd ~/rcboy/rcboy/avr && PATH=$PATH:~/.local/bin ~/.platformio/penv/bin/pio run --target upload"