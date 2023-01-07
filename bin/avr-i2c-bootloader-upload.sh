#!/bin/bash
echo "Syncing source code..."
rsync -r -ssh avr-i2c-bootloader bin include peta@10.0.0.38:/home/peta/devel/rcboy --delete
echo "Buiding avr-i2c-bootloader"
ssh peta@10.0.0.38 "cd ~/devel/rcboy/avr-i2c-bootloader && pio run --target upload"
