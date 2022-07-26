# BOM

- M1.4 screws for thumbstick
- black M2.5 screws for top 

# HW Design

- check if 1M pull-down resistor can be used for rpi power
- MIC should be centered around some decent voltage for internal reference and smaller gain perhaps
- check if VRPI can be sent to the joystick as well - no but we can add voltage divider 
- verify pads
- determine missing values - mic & speaker

# Hardware Checks

- test that ATTiny can read microphone
- check headphones detection working


- test rpi can read buttons 

# AVR Issues

- Status is only the first byte, then extra type for input properties, then extended state, then date time
- this way it will be easier to ask only for a few...
- reading the audio has lots of errors, probably too many irqs present...


- AVR software deboucing seems not to be very robust. See if this is breadboard related & fix
- micmax can't be aggregated like this since we need the max value actually

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

