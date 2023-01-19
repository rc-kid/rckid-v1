# RCBoy

- AVR speed is weird... Determine proper settings
- how to set fuses and how to set program file

## BOM

- M1.4 screws for thumbstick
- black M2.5 screws for top 
- extra mosfet switches (if available)
- extra buffers (if available)

## PCB

- move the USB breakout closer to the edge
- antenna might be too far, check that it fits

- check that headphones detection works with 2.5V as well
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

- Status is only the first byte, then extra type for input properties, then extended state, then date time
- this way it will be easier to ask only for a few...
- reading the audio has lots of errors, probably too many irqs present...

- micmax can't be aggregated like this since we need the max value actually

## RPI

- check JOY_BTN reading and AVR notification from the new pinout
- add microphone reading and check quality 

- turn off/stuff - https://github.com/raspberrypi/firmware/blob/13691cee95902d76bc88a3f658abeb37b3c90b03/boot/overlays/README#L1335, map to one of the UART pins 
- add joystick button direct reading

- can we speed things up with crosscompilation? https://deardevices.com/2019/04/18/how-to-crosscompile-raspi/

- multimedia player

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
