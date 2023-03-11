#!/bin/bash
# Update the config.txt 
sudo cp sd/config.txt /boot/config.txt

# install extra packages required by the tools and rckid itself
# sudo apt-get -y install xinit x11-xserver-utils pkg-config qt5-default libevdev-dev pigpio i2c-tools
sudo apt-get -y install pkg-config qt5-default libevdev-dev pigpio i2c-tools


# use wiringPi instead of pigpio wich polls all the time
sudo apt-get install wiringpi libi2c-dev 

# to make boot times slightly faster, disable the servics we are not using 
sudo systemctl disable dphys-swapfile.service
sudo systemctl disable keyboard-setup.service
sudo systemctl disable apt-daily.service
sudo systemctl disable triggerhappy.service
sudo systemctl disable avahi-daemon
# these too actually take a *lot*
sudo systemctl disable nmbd.service
sudo systemctl disable smbd.service
# also disable bluetooth
sudo systemctl disable hciuart-service
sudo systemctl disable bluetooth-service

# get the ili9341 driver and build it
cd ..
git clone https://github.com/juj/fbcp-ili9341.git
cd fbcp-ili9341
mkdir build
cd build
cmake -DILI9341=ON -DARMV8A=ON -DGPIO_TFT_DATA_CONTROL=25 -DGPIO_TFT_RESET_PIN=7 -DSPI_BUS_CLOCK_DIVISOR=10 -DDISPLAY_ROTATE_180_DEGREES=ON -DSTATISTICS=0 ..
make -j
cd ../../rckid
# enable as service so that the driver starts after reboot
sudo cp ~/rckid/sd/ili9341.service /lib/systemd/system/ili9341.service
sudo systemctl enable ili9341

# Install platformio and friens so that we can use the RPi to program the AVR responsible for power management and analog inputs
python3 -c "$(curl -fsSL https://raw.githubusercontent.com/platformio/platformio/master/scripts/get-platformio.py)"
sudo apt-get -y install python3-pip
pip3 install https://github.com/mraardvark/pyupdi/archive/master.zip

# Build rckid driver and main app from the repo. Build on single core so that we do not OOME, memory on RPi Zero is at premium. 
mkdir build
cd build
cmake .. -DARCH_RPI= -DCMAKE_BUILD_TYPE=Release
make 
# add the udev rule for the gamepad
cd ..
sudo cp sd/99-rckid-gamepad.rules /etc/udev/rules.d/99-rckid-gamepad.rules

# Turn off default retropie startup, instead start X with rckid as the only app
#sudo cp sd/autostart.sh /opt/retropie/configs/all/autostart.sh
#cp sd/.xinitrc /home/pi/.xinitrc
#sudo cp sd/.xinitrc /root/.xinitrc
