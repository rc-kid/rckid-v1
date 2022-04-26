#!/bin/bash
echo "Syncing source code..."
rsync -r -ssh src-avr include common pi@10.0.0.6:/home/pi/rcboy
echo "Buiding avr-src"
ssh pi@10.0.0.6 "cd ~/rcboy/src-avr && PATH=$PATH:~/.local/bin ~/.platformio/penv/bin/pio run --target upload"