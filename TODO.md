# RCBoy

## BOM

- M1.4 screws for thumbstick
- black M2.5 screws for top 
- extra mosfet switches (if available)
- extra buffers (if available)

## PCB

- check that headphones detection works with 2.5V as well
- check resized photores holes
- see if different accels can fit and be used/replaced under radio
- mic amplification is too great, 51k seems to do the trick
- verify thumbstick front panel hole placement
- check if 1M pull-down resistor can be used for rpi power
- audio board is too big to be inserted comfortably (most likely it is ok)
- ferrite beads are 1206, should be 0805, but now can also stay 1206

## AVR

- Status is only the first byte, then extra type for input properties, then extended state, then date time
- this way it will be easier to ask only for a few...
- reading the audio has lots of errors, probably too many irqs present...
- AVR software deboucing seems not to be very robust. See if this is breadboard related & fix
- micmax can't be aggregated like this since we need the max value actually

## RPI

- cleanup and comments on gui

- what is the stop-job at shutdown and how can we make shutdown faster & detect it on avr

- turn off/stuff - https://github.com/raspberrypi/firmware/blob/13691cee95902d76bc88a3f658abeb37b3c90b03/boot/overlays/README#L1335 -- can help? 
- multimedia player
- microphone reading

- unrecognized gamepad, check the udev rules? 
- if there are errors during startup a way to show them
- how to install pyupdi directly in platformio penv? so that I do not have to set PATH in tasks
- how should the volume buttons be reported on the gamepad, if at all
- debounce buttons
- joystick seems to work, but the responsivity is not the best and it seems some events are missed, check once not on breadboard I guess
- add repeat to gamepad keys (configurable)

# Remote 

## PCB

## AVR

- check that charging is turned off when charging is done

# Other

- accel keeps going dark... contacts? 

> https://martybugs.net/wireless/rubberducky.cgi -- site that seems to have decent info on antennas
