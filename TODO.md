# Questions

- transient voltage protection, which one to get?
- which antenna to buy? 
- how can screen brightness flicker in low battery mode be minimized? 
- pcb design ideas?
- mic amp design ok?
- clean voltage design ok?
- lego remote motor protection?  
- lego remote power, INA219 ok? 
- how to charge Li-Ion 2S? Is it worth it? 

## BOM

- M1.4 screws for thumbstick
- black M2.5 screws for top
- extra mosfet switches (if available)
- extra buffers (if available)

# RCKid

> A section about RCKid alone todos... Split into the schematic & PCB design, AVR code and RPi application and the RPi System in general (SD card settings mostly)

## PCB

- update JOYCON connector (!!)
- transorb should be before the charger for protection (!) 
- mic amplification is too great, 51k seems to do the trick
- ferrite beads are 1206, should be 0805, but now can also stay 1206
- decent I2C header

# AVR

- add errors (debug mode), alarm, recording, commands, etc. 
- add effects for rumbler & rgb 

## RPI - RayLib

- validate new pinout 
- add joystick button direct reading
- accelerometer is very noisy
- add microphone reading and check quality 
- if there are errors during startup a way to show them
- debounce buttons

- pixel editor as app?
- piano as app? 

## RPI - System

- disable serial port
- would it make sense to use fewer cores at higher speed? 

- turn off/stuff - https://github.com/raspberrypi/firmware/blob/13691cee95902d76bc88a3f658abeb37b3c90b03/boot/overlays/README#L1335, map to one of the UART pins 

- can we speed things up with crosscompilation? https://deardevices.com/2019/04/18/how-to-crosscompile-raspi/

- should button repeat be user-configurable? 
- reenable dpad and check that it works

# LEGO Remote

> Section about the lego remote part. Schematic & PCB and then AVR-only code. 

## PCB

- I2C memory for sound effects? 

## AVR

- write servo code & channels
- verify motors & servos
- actually write the code

# Other - proto

> https://martybugs.net/wireless/rubberducky.cgi -- site that seems to have decent info on antennas
