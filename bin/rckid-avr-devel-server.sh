#!/bin/bash
echo "Syncing source code..."
rsync -r -ssh rckid include bin peta@10.0.0.38:/home/peta/devel/rckid --delete
echo "Buiding avr-src"
ssh peta@10.0.0.38 "cd ~/devel/rckid/rckid/avr && pio run -e devel-server --target upload"
