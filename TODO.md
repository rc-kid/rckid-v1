# Hardware Checks

- test that ATTiny can be programmed from rpi
- test that ATTiny can signal rpi and I2C comms
- test that ATTiny can turn rpi on/off
- test that ATTiny can turn RGB on/off
- test that ATTiny can turn vibration on/off + PWM
- test that ATTiny can read joystick analog and the buttons
- test that ATTiny can PWM display brightness
- test that ATTiny can read microphone
- test that ATTiny can read photoresistor
- check if 1M pull-down resistor can be used for rpi power

- test that headphone detection works
- test rpi can read buttons 



- M1.4 brass inserts for thumbstick and M1.4 screws
- black M2.5 screws for top 
- get 5pin FPC for joysticks

- add jumper for turning rpi on
- check that 5V can control 3v3 for vibration motor
- check headphones detection working
- verify pads
- determine missing values

## Rpi image 

- how to install pyupdi directly in platformio penv? so that I do not have to set PATH in tasks

## Rpi - Gamepad & Drivers

- debounce buttons
- can use USB? Host/master?
- shared memory comms with other processes (dim display, battery level, temp, etc.)
- control display with PWM

- only use two axes for the accelerometer, figure out how to make gyro and accel to determine the propoer position

- joystick seems to work, but the responsivity is not the best and it seems some events are missed, check once not on breadboard I guess

# RPi - UI

- TODO

# AVR 

- check it can turn rpi on/off
- check rpi can program it, should there be a way to force RPI on via some button too to override the shutdown? 

## NRF24L01+

- there's an issue with SPI, the breadboard is pretty unstable affair. Other than that seems to work. Antenna is necessary. I think the rubber duckie can be stripped and added in the case easily. The connector can be semi-exposed to allow for longer range antennas for the walkie-talkie, if ever necessary. Range must be properly tested. 

> https://martybugs.net/wireless/rubberducky.cgi -- site that seems to have decent info on antennas

