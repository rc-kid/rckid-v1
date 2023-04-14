# SD Image Setup

> TODO provide a download of the full rpi image as part of the project with all this setup. Follow this windowde only when building new image. 

Download RaspberryPi OS Imager and select the Retropie distribution for Raspberry pi 2 W. Flash the image on the SD card and then reinsert the card. Its `boot` partition will be mounted. Edit the default networks the RPI should connect to in the `sd/wpa_supplicant.conf` (higher priority is better). Then copy the `wpa_supplicant.conf` and `ssh` files from the `sd` directory to the boot partition on the SD card. Insert the card to rpi and power on. 

> The extra settings dialog accessible with `c-s-X` does not seem to actually do anything, which is why the above steps are done manually. (Manual settings from [here][https://retropie.org.uk/docs/Wifi/])

To connect to the board, determine its ip address and ssh with username `pi` and password `raspberry`. 

Then run the following:

    sudo apt-get update
    sudo apt-get upgrade
    sudo apt-get install mc htop tmux git cmake
    git clone git@github.com:zduka/rckid.git

Then run `sudo racpi-config`, enable audio jack audio and i2c, disable the experimental gl driver. 

To configure _retropie_, run the following:

    sudo ~/RetroPie-Setup/retropie_setup.sh


When done, run the rest of the SD image setup (see the script file for details):

    cd rckid
    bash sd/install.sh

Then run `sudo raspi-config` and make the following changes:

- disable wait for the network during boot
- disable tty on serial console
- set audio output to headphones

# Startup Time

To analyze startup time run the following commands for the overall time, for the longest hogs and for a pretty plot:

    systemd-analyze
    systemd-analyze blame
    systemd-analyze plot > plot.svg




