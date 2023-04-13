# Questions

- which antenna to buy? 
- PWM motor control frequency - 20kHz should be enough

## BOM

- M1.4 screws for thumbstick
- black M2.5 screws for top
- better 3.0V LDO (cleaner, higher voltage better for mic & speed, such as https://cz.mouser.com/ProductDetail/Texas-Instruments/TPS7A2030PDBVR?qs=hd1VzrDQEGgZMtQinkbhYw%3D%3D)
- solder tips, solder flux
- 5pin 0.5mm pitch connector (https://cz.mouser.com/ProductDetail/Molex/505278-0533?qs=c8NFF48pVsCY0CQNgl3Xjw%3D%3D or https://cz.mouser.com/ProductDetail/Molex/505110-0592?qs=RawsiUxJOFR577kELw3Dww%3D%3D)
- ATTiny1616

# RCKid

> A section about RCKid alone todos... Split into the schematic & PCB design, AVR code and RPi application and the RPi System in general (SD card settings mostly)

## PCB

- should the ps1000 hole be permanent?, likely yes - might also prevent the joycon encosure wobble 

# AVR

- check that long home works in repair mode as well
- add low and critical battery warnings and proper actions
- add effects for rumbler & rgb 
- add mic level wakeup and others
- add alarm wakeup
- add reset cause detection

- ensure that the modes and powering on & off works reliably so that we can switch off the I2C display and test comms

## RPI - RayLib

- header
- add a way for the menu / submenu to detect it's been detached
- add torchlight widget
- webserver from the app - see https://github.com/yhirose/cpp-httplib ?

- how to configure, via JSON I guess, reuse the simple UI from mp3 player should be enough
- samba on demand? etc.

- add rumbler / sound for ui actions

- verify AVR comms
- accelerometer is very noisy
- add microphone reading and check quality 
- if there are errors during startup a way to show them
- debounce buttons

- piano as app?
- sound recorder as app?  

- can control easily via Telegram by sending messages and stuff to bot channels (!)
- heart bar for time spent, track time spent when widgets are in foreground

- how to run rckid at startup with the fcbp
- how multiple apps can share framebuffer


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
- allow to set servo min & max pulse and then interpolate between them for more precission control

# Other - proto

> https://martybugs.net/wireless/rubberducky.cgi -- site that seems to have decent info on antennas
