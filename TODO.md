# Questions

- transient voltage protection, which one to get? (up to 10v)
- which antenna to buy? 
- mic amp design ok? - no
- lego remote motor protection?  - led
- how to charge Li-Ion 2S? Is it worth it? - battery ballancers exist
- PWM motor control frequency - 20kHz should be enough

## BOM

- M1.4 screws for thumbstick
- black M2.5 screws for top
- extra mosfet switches (if available)
- extra buffers (if available)
- better 3.0V LDO (cleaner, higher voltage better for mic & speed, such as https://cz.mouser.com/ProductDetail/Texas-Instruments/TPS7A2030PDBVR?qs=hd1VzrDQEGgZMtQinkbhYw%3D%3D)
- solder tips, solder flux
- kapton tape? 
- 5pin 0.5mm pitch connector (https://cz.mouser.com/ProductDetail/Molex/505278-0533?qs=c8NFF48pVsCY0CQNgl3Xjw%3D%3D or https://cz.mouser.com/ProductDetail/Molex/505110-0592?qs=RawsiUxJOFR577kELw3Dww%3D%3D)
- ATTiny1616

# RCKid

> A section about RCKid alone todos... Split into the schematic & PCB design, AVR code and RPi application and the RPi System in general (SD card settings mostly)

## PCB

- can there be room for MAX9814 breakout with mic? 
- right angle buttons on the bottom instead of the boards
- cutout possibility & mounting holes for PSP1000 thumbstick instead of joycon
- verify joycon connection
- mic amplification is too great, 51k seems to do the trick
- ferrite beads are 1206, should be 0805, but now can also stay 1206
- decent I2C header

# AVR

- ensure that the modes and powering on & off works reliably so that we can switch off the I2C display and test comms
- add errors (debug mode), alarm, recording, commands, etc. 
- add effects for rumbler & rgb 

## RPI - RayLib

- validate new pinout for NRF & buttons
- very AVR comms
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

- maybe use only 0..63 values for the motor speed with higher frequency, then whole motor fits in single byte 
- write servo code & channels
- verify motors & servos
- actually write the code

# Other - proto

> https://martybugs.net/wireless/rubberducky.cgi -- site that seems to have decent info on antennas
