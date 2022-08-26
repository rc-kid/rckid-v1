# PCB


- enlarge the pads on the backs of the shoulder so that they can be easily soldered at the bottom
- verify thumbstick front panel hole placement
- test if different buffer can be used because the one I have is hard to find, need to find something that is possible
- the UPDI has a pullup so in theory it brings rpi gpio to more than 3v3 and can't work this way. TXB0101 seems to work for the level shifting of the updi line. Alternatively we can give up on the rpi programming the avr, which is though a bit of shame. Is there enough pins in the design? 
- if vclean is brought down to 2.5V then reading joystick is simpler and we can run the clean voltage from battery level as low as 3.4 - see if it is loud enough, it just might
- headphones on/off does not work, the voltage when headphones are inserted is too small


- audio board is too big to be inserted comfortably (most likely it is ok)
- ferrite beads are 1206, should be 0805, but now can also stay 1206
- test if the new mosfets are like DMP1045 (most likely yes)

# BOM

- M1.4 screws for thumbstick
- black M2.5 screws for top 
- TXB0101
- switches 
- extra mosfet switches (if available)
- extra buffers (if available)

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

- how should the volume buttons be reported on the gamepad, if at all

- debounce buttons
- can use USB? Host/master?
- shared memory comms with other processes (dim display, battery level, temp, etc.)
- control display with PWM

- only use two axes for the accelerometer, figure out how to make gyro and accel to determine the propoer position

- joystick seems to work, but the responsivity is not the best and it seems some events are missed, check once not on breadboard I guess

## NRF24L01+

> https://martybugs.net/wireless/rubberducky.cgi -- site that seems to have decent info on antennas

