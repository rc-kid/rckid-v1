# RCBoy

## BOM

- M1.4 screws for thumbstick
- black M2.5 screws for top 
- extra mosfet switches (if available)
- extra buffers (if available)

## PCB

- move joystick btn from AVR to RPI  
- add pin connection from rpi to AVR for detecting shutdown
- move the USB breakout closer to the edge

- check that headphones detection works with 2.5V as well
- see if different accels can fit and be used/replaced under radio
- mic amplification is too great, 51k seems to do the trick
- verify thumbstick front panel hole placement
- check if 1M pull-down resistor can be used for rpi power
- audio board is too big to be inserted comfortably (most likely it is ok)
- ferrite beads are 1206, should be 0805, but now can also stay 1206

## Bootloader

- enable backlight
- pull AVR_IRQ low when working
- write the programmer
- test by verifying the bootloader code to be what is expected

## AVR

- create I2C bootloader to save the serial pins and provide uploading capabilities from RPI
- https://docs.platformio.org/en/latest/platforms/atmelavr.html#bootloader-programming
- https://www.microchip.com/en-us/application-notes/an2634

- Status is only the first byte, then extra type for input properties, then extended state, then date time
- this way it will be easier to ask only for a few...
- reading the audio has lots of errors, probably too many irqs present...
- AVR software deboucing seems not to be very robust. See if this is breadboard related & fix
- micmax can't be aggregated like this since we need the max value actually

## RPI

- turn off/stuff - https://github.com/raspberrypi/firmware/blob/13691cee95902d76bc88a3f658abeb37b3c90b03/boot/overlays/README#L1335 -- can help? Not really - perhaps tell systemd to execute some app? 
- multimedia player
- microphone reading

- if there are errors during startup a way to show them
- debounce buttons
- joystick seems to work, but the responsivity is not the best and it seems some events are missed, check once not on breadboard I guess (?)

- should button repeat be user-configurable? 
- reenable dpad and check that it works

# Remote 

## PCB

## AVR

- check that charging is turned off when charging is done

# Other

- accel keeps going dark... contacts? 

> https://martybugs.net/wireless/rubberducky.cgi -- site that seems to have decent info on antennas
