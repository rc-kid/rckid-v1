# PCB Checks

- mic amplification is too great, 51k seems to do the trick
- verify thumbstick front panel hole placement
- check if 1M pull-down resistor can be used for rpi power

# PCB Updates

- resize photoresistor holes

# PCB Old

- audio board is too big to be inserted comfortably (most likely it is ok)
- ferrite beads are 1206, should be 0805, but now can also stay 1206

# BOM

- M1.4 screws for thumbstick
- black M2.5 screws for top 
- extra mosfet switches (if available)
- extra buffers (if available)

# AVR Issues

- Status is only the first byte, then extra type for input properties, then extended state, then date time
- this way it will be easier to ask only for a few...
- reading the audio has lots of errors, probably too many irqs present...


- AVR software deboucing seems not to be very robust. See if this is breadboard related & fix
- micmax can't be aggregated like this since we need the max value actually

## Rpi image 

- unrecognized gamepad, check the udev rules? 

- how to install pyupdi directly in platformio penv? so that I do not have to set PATH in tasks

## Rpi - Gamepad & Drivers

- how should the volume buttons be reported on the gamepad, if at all

- debounce buttons

- only use two axes for the accelerometer, figure out how to make gyro and accel to determine the propoer position

- joystick seems to work, but the responsivity is not the best and it seems some events are missed, check once not on breadboard I guess

## NRF24L01+

> https://martybugs.net/wireless/rubberducky.cgi -- site that seems to have decent info on antennas



## RPI App

- rewrite the menu hierarchy so that lazy menu is better supported
- make footer and header directly part of GUI, no need for separate classes
- down selects, up goes back in the menu hierarchy for nicer shortcuts 
- add gauge control

- then start playing with avr interop (brightness, etc.)
- turn off/stuff
- multimedia player
- microphone reading
