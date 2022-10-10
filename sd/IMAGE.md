# SD Image Setup

> TODO provide a download of the full rpi image as part of the project with all this setup. Follow this guide only when building new image. 

Download RaspberryPi OS Imager and select the Retropie distribution for Raspberry pi 2 W. Then press `c-s-X` before writing to the immage to set additional options. Enable SSH, set user password and set WiFi settings. 

> The extra settings seem not to work for retropie. Check this: https://retropie.org.uk/docs/Wifi/


Password raspberry

When written, boot up the rpi, wait for a bit and then ssh to it and run the following commands to update the rpi:

    sudo apt-get update
    sudo apt-get upgrade
    
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

