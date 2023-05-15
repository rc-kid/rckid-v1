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

- wrong holes wrt center of the joystick
- not enough room for the battery

- should the ps1000 hole be permanent?, likely yes - might also prevent the joycon encosure wobble 
- move the trace away from the ps1000 hole too
- enlarge L and R vertical pin hole pads
- VCC test point
- joystick button test point
- proper hole for the thumbstick

- check NRF24, check RGB, check rumbler
- might also use [this](http://k-silver.com/html_products/JP19%EF%BC%88%E6%AD%A3%E6%8F%92%E8%93%9D%E8%89%B2%E6%91%87%E6%9D%86%EF%BC%89-833.html) thunbstick, that can be bought from [Adafruit](https://www.adafruit.com/product/5628) - will be much easier to solder, might require special pcb board or some such

## Bootloader

- allow to start bootloader by setting EPROM bits
- cleanup & extend the programmer code so that it can dump the stored files, read/write EEPROM and so on

## AVR

- check that long home works in repair mode as well
- add low and critical battery warnings and proper actions
- add effects for rumbler & rgb 
- add mic level wakeup and others
- add alarm wakeup
- add reset cause detection

- ensure that the modes and powering on & off works reliably so that we can switch off the I2C display and test comms

## RPI - RayLib

> To be tested/bugs: 

> I/O items:

- rename buttons and aces according to logitech
- make keyboard new device
^- see if after this we can actually use the joystick well

- retroarch is not loading correct cfg
- figure out how to configure the joystick manually 
- retroarch mapping is beyond weird - not sure how to fix
- key repeat is too fast
- debounce buttons

> Minor code issues & features to implement

- add == to Value and double parsing
- add JSON backed menu that allows reordering
- how to configure, via JSON I guess,
- samba on demand? etc.

> Sensors 

- accelerometer is very noisy
- add accel temperature processing
- add photoresistor processing

> Audio 

- add player 
- add microphone reading and check quality 

> More apps:

- sound recorder as app?  
- add torchlight widget




- actually do the audio, video menu
- on pop for menu so that we can clear it, should we want to
- modal errors & stuff
- keep icon instead of text in gauges
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
