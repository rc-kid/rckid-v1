# SD Image Setup

> TODO provide a download of the full rpi image as part of the project with all this setup. Follow this guide only when building new image. 

Download RaspberryPi OS Imager and select the Retropie distribution for Raspberry pi 2 W. Flash the image on the SD card and then reinsert the card. Its `boot` partition will be mounted. Edit the default networks the RPI should connect to in the `sd/wpa_supplicant.conf` (higher priority is better). Then copy the `wpa_supplicant.conf` and `ssh` files from the `sd` directory to the boot partition on the SD card. Insert the card to rpi and power on. 

> The extra settings dialog accessible with `c-s-X` does not seem to actually do anything, which is why the above steps are done manually. (Manual settings from [here][https://retropie.org.uk/docs/Wifi/])

To connect to the board, determine its ip address and ssh with username `pi` and password `raspberry`. 

Then run the following:

    sudo apt-get update
    sudo apt-get upgrade
    sudo apt-get install mc htop tmux git cmake
    git clone git@github.com:zduka/rcboy.git

When done, run the rest of the SD image setup (see the script file for details):

    cd rcboy
    bash sd/install.sh





Password raspberry

When written, boot up the rpi, wait for a bit and then ssh to it and run the following commands to update the rpi:

    
Then install what we need, first some basic utilities to keep me happy in the image:

    sudo apt-get install mc htop tmux git cmake

Get rcboy:

    git clone git@github.com:zduka/rcboy.git

Install the ili9341 driver and build it:

    git clone https://github.com/juj/fbcp-ili9341.git
    cd fbcp-ili9341
    mkdir build
    cd build
    cmake -DILI9341=ON -DARMV8A=ON -DGPIO_TFT_DATA_CONTROL=25 -DGPIO_TFT_RESET_PIN=7 -DSPI_BUS_CLOCK_DIVISOR=10 -DDISPLAY_ROTATE_180_DEGREES=ON -DSTATISTICS=0 ..
    make -j

Add fcbp as a service:

    sudo cp ~/rcboy/sd/ili9341.service /lib/systemd/system/ili9341.service
    sudo systemctl enable ili9341

Software necessary for the rcboy app:

    sudo apt-get install xinit x11-xserver-utils pkg-config qt5-default libevdev-dev pigpio



## Software installation

    sudo apt-get install xinit x11-xserver-utils
    sudo apt-get install qt5-default libqt5gamepad5-dev
    sudo apt-get install libevdev-dev pkg-config



## Starting the gui app 

Edit `/opt/retropie/configs/all/autostart.sh` to do the following:

    startx -s 0

Then edit `/home/pi/.xinitrc` to start the rcboy app and configure the xserver properly:

    xset s off         # don't activate screensaver
    xset -dpms         # disable DPMS (Energy Star) features.
    xset s noblank     # don't blank the video device
    /home/pi/rcboy/build/rcboy/rpi/rcboy/rcboy

