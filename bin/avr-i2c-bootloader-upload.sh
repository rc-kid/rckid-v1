#!/bin/bash
echo "Syncing source code..."
rsync -r -ssh avr-i2c-bootloader bin include peta@10.0.0.38:/home/peta/devel/rckid --delete
#rsync -r -ssh avr-i2c-bootloader bin include pi@10.0.0.39:/home/pi/rckid --delete
echo "Buiding avr-i2c-bootloader"
ssh peta@10.0.0.38 "cd ~/devel/rckid/avr-i2c-bootloader && pio run --target upload"
#ssh pi@10.0.0.39 "cd ~/rckid/avr-i2c-bootloader && PATH=\$PATH:~/.local/bin ~/.platformio/penv/bin/pio run --target upload"
