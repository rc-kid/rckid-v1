# Checklist

- [X] gamepad support in retroarch
- [ ] analog axes
- [ ] clean controls
- [X] basic games working
- [X] basic video working
- [X] basic audio playing
- [ ] DOSBox games basics
- [ ] remote (NRF)
- [ ] Walkie-Talkie

# Questions

- PWM motor control frequency - 20kHz should be enough
- AVR chips keep getting bricked... No clue why:(

## BOM

- M1.4 screws for thumbstick
- better 3.0V LDO (cleaner, higher voltage better for mic & speed, such as https://cz.mouser.com/ProductDetail/Texas-Instruments/TPS7A2030PDBVR?qs=hd1VzrDQEGgZMtQinkbhYw%3D%3D)
- solder tips, solder flux
- 5pin 0.5mm pitch connector (https://cz.mouser.com/ProductDetail/Molex/505278-0533?qs=c8NFF48pVsCY0CQNgl3Xjw%3D%3D or https://cz.mouser.com/ProductDetail/Molex/505110-0592?qs=RawsiUxJOFR577kELw3Dww%3D%3D)
- ATTiny1616

# RCKid

> A section about RCKid alone todos... Split into the schematic & PCB design, AVR code and RPi application and the RPi System in general (SD card settings mostly)

## PCB

- make more room for the battery (verify)
- check that the battery / usb switch works fine (add cap?)
- add mounting holes and fixes for supports
- make RGB face top (?)
- verify joycon solder pad placement before ordering

- might also use [this](http://k-silver.com/html_products/JP19%EF%BC%88%E6%AD%A3%E6%8F%92%E8%93%9D%E8%89%B2%E6%91%87%E6%9D%86%EF%BC%89-833.html) thunbstick, that can be bought from [Adafruit](https://www.adafruit.com/product/5628) - will be much easier to solder, might require special pcb board or some such

## Bootloader

- cleanup & extend the programmer code so that it can dump the stored files, read/write EEPROM and so on

## AVR

- DIFFERENT PIN MAPPING FOR NEW VERSION
- ensure home button long press cannot be spurious
- waking up works only every second time
- add effects for rumbler & rgb 
- add mic level wakeup and others
- add alarm wakeup

## RPI - RayLib

- DIFFERENT PIN MAPPING FOR NEW VERSION

- second recording does not seem to work

- start sending to / reading from NRF
- change recording visualization to something easier to draw. Maybe just less granularity
- spurious buttons after recording end


- the GPU performance is well... bad. 
- check if using VCDispmanX, or SDL can improve things. 
- check that we are find detecting first boot with it

- die if in bootloader mode 
- read and reset the error in debug at the beginning of the app

> To be tested/bugs: 

- ensure config on fresh start from clean sd

> I/O items:

- key repeat is too fast
- debounce buttons

> Minor code issues & features to implement

- add == to Value and double parsing
- add JSON ostream << for array and struct
- add JSON backed menu that allows reordering
- how to configure, via JSON I guess,
- samba on demand? etc.

> Sensors 

- accelerometer is very noisy
- add accel temperature processing
- add photoresistor processing

> Audio 

- add player 
- add recorder app

> More apps:

- add torchlight widget



- handle errors in player configs
- actually do the audio, video menu
- on pop for menu so that we can clear it, should we want to
- modal errors & stuff
- add a way for the menu / submenu to detect it's been detached
- add rumbler / sound for ui actions
- if there are errors during startup a way to show them
- can control easily via Telegram by sending messages and stuff to bot channels (!)
- heart bar for time spent, track time spent when widgets are in foreground

> Low priority (or even abandoned)

- webserver from the app - see https://github.com/yhirose/cpp-httplib ?
- piano as app?

## RPI - System

- disable serial port
- would it make sense to use fewer cores at higher speed? 

- turn off/stuff - https://github.com/raspberrypi/firmware/blob/13691cee95902d76bc88a3f658abeb37b3c90b03/boot/overlays/README#L1335, map to one of the UART pins 

- can we speed things up with crosscompilation? https://deardevices.com/2019/04/18/how-to-crosscompile-raspi/

- should button repeat be user-configurable? 

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
