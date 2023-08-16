#!/bin/bash
echo "Syncing source code..."
rsync -r -ssh rckid include pi@10.0.0.40:/home/pi/rckid --delete
echo "Buiding avr-src"
ssh pi@10.0.0.40 "cd ~/rckid/rckid/avr && PATH=\$PATH:~/.local/bin ~/.platformio/penv/bin/pio run -e rckid-avr --target upload"
