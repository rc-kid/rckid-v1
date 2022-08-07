# SD Image Setup

## Software installation

    sudo apt-get install xinit x11-xserver-utils
    sudo apt-get install libqt5gamepad5-dev



## Starting the gui app 

Edit `/opt/retropie/configs/all/autostart.sh` to do the following:

    startx -s 0

Then edit `/home/pi/.xinitrc` to start the rcboy app and configure the xserver properly:

    xset s off         # don't activate screensaver
    xset -dpms         # disable DPMS (Energy Star) features.
    xset s noblank     # don't blank the video device
    /home/pi/rcboy/build/rcboy/rpi/rcboy/rcboy

