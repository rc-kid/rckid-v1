# RCKid

> The vertical version with DPAD


- change pin mapping on ATTiny
- update the joycon connector
- determine enclosure for joystick
- can audio PCB be simplified? 


# PCB v2

- less complicated design (fewer boards)
- better access to the SD card
- being able to disconnect the battery via cable
- fit an inductor to the screen brightness driver for better performance in low batt
- possibly better layout

# RCKid

- AVR speed is weird... Determine proper settings

- software: raylib, lvgl, sfml, etc. See if I can write directly to fb and how soon can I do so - technically I should be able to do this as soon as the TFT driver starts

## BOM

- M1.4 screws for thumbstick
- black M2.5 screws for top 
- extra mosfet switches (if available)
- extra buffers (if available)

## PCB

- should the accel be moved so that there is space for the nrf connector? 
- verify thumbstick front panel hole placement (and thumbstick pattern dimensions)

- check the new position of the usb
- antenna might be too far, check that it fits

- mic amplification is too great, 51k seems to do the trick
- check if 1M pull-down resistor can be used for rpi power
- audio board is too big to be inserted comfortably (most likely it is ok)
- ferrite beads are 1206, should be 0805, but now can also stay 1206

## AVR

- thumbstick reading does not seem to be working properly
- joystick button detection
- reading the audio has lots of errors, probably too many irqs present... (MAYBE RESOLVED)
- micmax can't be aggregated like this since we need the max value actually

## RPI-RayLib

- accelerometer is very noisy

## RPI

- disable serial port
- would it make sense to use fewer cores at higher speed? 
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

# Other - proto

> https://martybugs.net/wireless/rubberducky.cgi -- site that seems to have decent info on antennas
