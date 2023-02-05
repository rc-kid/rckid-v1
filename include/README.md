# Common Platform and Devices

This is a header only implementation of a common interface that can be shared across the MCUs I work with (namely various AVRs, RP2040, ESP8266) for the peripherals I use. All code is portable and header only. 

## RaspberryPi

There are multiple libraries that can be used to deal with the gpio from C/C++. The [`pigpio`](http://abyz.me.uk/rpi/pigpio/) is fairly decent, but thanks to the constant polling of the gpio pins uses non-trivial amounts of CPU. The [`wiringPi`](http://wiringpi.com/) seems to be much older and less robust, but its gpio implementation does not use the CPU. The library does not provide as nice to use I2C and SPI functions, so the rpi platform implements those from scratch using linux character devices. 

## Mock

