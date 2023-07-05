#!/bin/bash
echo "Syncing source code..."
rsync -r -ssh nrf-repeater include bin peta@10.0.0.38:/home/peta/devel/rckid --delete
echo "Buiding nrf-repeater"
ssh peta@10.0.0.38 "cd ~/devel/rckid/nrf-repeater && pio run --target upload"
