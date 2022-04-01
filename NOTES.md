# Installing on RPI Zero

> https://gist.github.com/gbaman/50b6cca61dd1c3f88f41
> https://gist.github.com/gbaman/975e2db164b3ca2b51ae11e45e8fd40a
> https://learn.adafruit.com/turning-your-raspberry-pi-zero-into-a-usb-gadget

In `/boot/config.txt` make the following changes:

    framebuffer_width=320
    framebuffer_height=240
    camera_auto_detect=0
    display_auto_detect=0
    #dtoverlay=vc4-kms-v3d
    dtoverlay=dwc2
    hdmi_force_hotplug=1
    hdmi_cvt=320 240 60 1 0 0 0
    hdmi_group=2
    hdmi_mode=87

Then in `/boot/cmdline.txt` insert `modules-load=dwc2,g_ether` after rootwait.

    sudo systemctl disable dhcpcd

Then edit `/etc/network/interfaces` to this:

    auto lo
    iface lo inet loopback

    auto usb0
    allow-hotplug usb0
    iface usb0 inet static
        address 192.168.137.2
        netmask 255.255.255.0
        gateway 192.168.137.1

Now we have internet:

    sudo apt-get update
    sudo apt-get upgrade

## ILI9341 SPI Display

> https://github.com/juj/fbcp-ili9341

    sudo apt-get install cmake git
    git clone https://github.com/juj/fbcp-ili9341.git
    cd fbcp-ili9341
    mkdir build
    cd build
    cmake -DILI9341=ON -DARMV8A=ON -DGPIO_TFT_DATA_CONTROL=25 -DGPIO_TFT_RESET_PIN=7 -DSPI_BUS_CLOCK_DIVISOR=10 -DDISPLAY_ROTATE_180_DEGREES=ON ..
    make -j
    sudo ./fbcp-ili9341

Then add the ili9341.service file to /lib/systemd/system and run

    sudo systemctl enable ili9341

The speed factor of 10 seems to be ok on rpi zero 1, pretty stable 40MHz. 

> Claims to only work in 32bit version - can be ported to 64bit?  

## I2C

just enable `dtoverlay=i2c` in config.txt

## SPI

The display takes over SPI0. To use SPI1, it must be enabled explicitly by adding the following to config.txt:

    dtoverlay=spi1-2cs,cs0_pin=16,cs1_pin=26

Then the devices should be available as:

    /dev/spidev1.0
    /dev/spidev1.1

Do not enable the default spi0 as it is not needed by the framebuffer driver, it talks to the chip directly.

> Can this be used for a splashscreen? 

# PWM Audio

Add the following to config.txt:

    dtoverlay=audremap,enable_jack=on

Then reboot and in raspi-config set headphones as audio output. 

> https://learn.adafruit.com/introducing-the-raspberry-pi-zero/audio-outputs (a RC filter to add so that we have headphones). Then TPA311 for the speaker

On breadboard the audio is of pretty low quality. Perhaps use LDO for the PCB? Can also utilize the buffer circuit for newer rpi models (see adafruit above)

## Microphone

Can't use I2S as its pins conflict with SPI1, which is absolutely needed. Options:

- use the AVR as poor man's buffer and ADC, transfer over I2C 
- get an usb microphone

## Other HW

- WiFi via RPI, SX1278 LoRa, NRF24l01p
- 3000mAh battery from rpishop
- USB-C battery charger, or is it reasonable to use USB as OTG mass storage? 
- accelerometer

- the best possible version with 3.2" display, then also a lower cost device with 2.8" display, maybe different battery and case. 



# Software

    sudo apt-get install mc htop tmux vlc

Use cvlc for playing videos, can play decent scaled FullHD



## Boot time 

It wastes energy a bit, but can be useful especially with recessed select & start buttons is to power on raspberry pi as soon as the button is pressed so that the long press counts towards the boot time. If the button is released too soon, kill power to RPI. The root FS overlay needs to exist for this. 

    disable_splash=1
    boot_delay=0

> https://singleboardbytes.com/637/how-to-fast-boot-raspberry-pi.htm


    systemd-analyze
    systemd-analyze blame
    systemd-analyze plot > plot.svg

In `raspi-config` select not to wait for the network during boot. 

Disable unnecessary services:

    sudo systemctl disable dphys-swapfile.service
    sudo systemctl disable keyboard-setup.service
    sudo systemctl disable apt-daily.service
    sudo systemctl disable triggerhappy.service
    sudo systemctl disable avahi-daemon

Disable bluetooth:

    sudo systemctl disable hciuart-service
    sudo systemctl disable bluetooth-service

Make the SD card faster:

    dtparam=sd_overclock=100
    
    






## Building Retroarch

Actually don't bother with building retroarch, use retropie which already has everything built-in and simply run the games using retroarch's commandline:

    /opt/retropie/emulators/retroarch/bin/retroarch -L /opt/retropie/libretrocores/lr-snes9x2010/snes9x2010_libretro.so /home/pi/roms/snes/Sonic\ the\ Hedgehog\ \(Unl\)\ \[p1\].smc

> how to configure retroarch for the joystick? 

Other emulators must be downloaded, preferrably as libretro cores so that I can make things small. Installable via retropie_setup

Dosbox is not libretro core it seems but the emulator can definitely be installed. Wacky Wheels work after mounting (mount c /home/pi/roms/dosbox), so there is hope. Dosbox does not work from SSH.

# Inputs

- to inspect various controllers: https://github.com/Grumbel/evtest-qt/
- uinput might be used to generate the events, maybe even including keyboard






# GPIO

    sudo apt-get install pygpio

