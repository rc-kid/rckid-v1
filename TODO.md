# Hardware Checks

- check if VRPI can be sent to the joystick as well
- MIC should be centered around some decent voltage for internal reference and smaller gain perhaps

- test that ATTiny can signal rpi and I2C comms
- test that ATTiny can turn rpi on/off
- test that ATTiny can turn vibration on/off + PWM
- test that ATTiny can read joystick analog and the buttons
- test that ATTiny can read microphone
- test that ATTiny can read photoresistor
- check if 1M pull-down resistor can be used for rpi power

- test that headphone detection works
- test rpi can read buttons 



- M1.4 brass inserts for thumbstick and M1.4 screws
- black M2.5 screws for top 

- add jumper for turning rpi on (or maybe not necessary, just a test point)
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

## NRF24L01+

> https://martybugs.net/wireless/rubberducky.cgi -- site that seems to have decent info on antennas

