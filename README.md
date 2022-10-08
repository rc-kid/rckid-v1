# RCBoy

Handheld device for kids that provides the following:

- remote control for lego models & other stuff
- walkie talkie
- retro games player
- music / video player

Based on RPi Zero 2W, SPI display and nrf24l01p and/or sx1278 for comms

# Software

## Main GUI

Runs on RPI, games, music, controllers, etc. 

## ATTiny

ATTiny is always on, but mostly sleeping. Talks to the RPI via I2C and 


# SD Card Image

In raspi-config:

- disable serial port login
- enable I2C


Install the development dependencies:

    sudo apt-get install libevdev-dev
    sudo apt-get install pigpio

Add the udev rule for the gamepad to be recognized as a joystick so that emulation station and retroarch pick it up by copying the `rpi/gamepad/99-rcboy-gamepad.rules` file to `/etc/udev/rules/d`. 

> TODO verify with new image whether the libi2c-dev is needed when using pygpio

Install platformio & friends:

    python3 -c "$(curl -fsSL https://raw.githubusercontent.com/platformio/platformio/master/scripts/get-platformio.py)"
    sudo apt-get install python3-pip
    pip3 install https://github.com/mraardvark/pyupdi/archive/master.zip

> Note that pyupdi is installed in `~/.local/bin` which is not in path generally.

Add the `rpi/ili9341/ili9341.service` to `/lib/systemd/system` and run:

    sudo systemctl enable ili9341

# Build

To launch a serial console, connect the serial to USB adapter to the TX RX and GND pins (for my setup it is white/green/black wires) and then launch the following command in terminal:

    minicom -b 115200 -o -D /dev/ttyUSB0

## Extras

- real time clock
- vibration motor
- buttons (maybe 3d printed caps)
- small and nice speaker
- i2c flash memory, or can use the memory on AVR to store data between on/off

## PCB

### Test Points

TP1  | VUSB
TP2  | VBATT
TP3  | VCC
TP4  | VRPI
TP5  | Photoresistor 
TP6  | Mic
TP7  | Charge

https://www.flaticon.com/

<div>Icons made by <a href="https://www.freepik.com" title="Freepik">Freepik</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/pixel-perfect" title="Pixel perfect">Pixel perfect</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/vectors-market" title="Vectors Market">Vectors Market</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/srip" title="srip">srip</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/monkik" title="monkik">monkik</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/aldo-cervantes" title="Aldo Cervantes">Aldo Cervantes</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/smashicons" title="Smashicons">Smashicons</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/umeicon" title="Umeicon">Umeicon</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/bukeicon" title="bukeicon">bukeicon</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/yogi-aprelliyanto" title="Yogi Aprelliyanto">Yogi Aprelliyanto</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/good-ware" title="Good Ware">Good Ware</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/apien" title="apien">apien</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/surang" title="surang">surang</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/chahir" title="chahir">chahir</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div>
