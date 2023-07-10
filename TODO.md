> Currently in progress/working on. All projects combined. When removing from here make sure to check the sections below for long term items and remove them as needed. 

- battery on: 9:30
- charging: 9:45 - 11:22

- charging light does not seem to work that well
- make the charging light work even when RPi off / sleeping (check)
- add sin based interleaving (? - would it be enough)
- maybe do just 1sec quick bursts (easier in sleep)
- switch charging control single pullup 

- change recording visualization to something easier to draw. Maybe just less granularity

> Things to buy / test. 

## Shopping List

- buy some more NRFs (not the cheapest ones)
- https://martybugs.net/wireless/rubberducky.cgi -- site that seems to have decent info on antennas
- M1.4 screws for thumbstick
- better 3.0V LDO (cleaner, higher voltage better for mic & speed, such as https://cz.mouser.com/ProductDetail/Texas-Instruments/TPS7A2030PDBVR?qs=hd1VzrDQEGgZMtQinkbhYw%3D%3D)
- solder tips, solder flux
- 5pin 0.5mm pitch connector (https://cz.mouser.com/ProductDetail/Molex/505278-0533?qs=c8NFF48pVsCY0CQNgl3Xjw%3D%3D or https://cz.mouser.com/ProductDetail/Molex/505110-0592?qs=RawsiUxJOFR577kELw3Dww%3D%3D)
- ATTiny1616
- buy extra RPi Zero 2W's (2-3), that way I have parts to build almost 6 RCKids (modulo joysticks)
- buy extra SPI Displays (3-4)

> TODOs for the various projects that RCkid consists of. Each project has its own todo list.

## RCKid PCB & Case

- make more room for the battery (verify)
- check that the battery / usb switch works fine (add cap?)
- add mounting holes and fixes for supports
- make RGB face top (?)
- verify joycon solder pad placement before ordering
- check charging detection works, can perhaps ignore the pullup/downs? into middle and use just one pullup/down? Needs external pull-up. Only works when USB is present - need better USB checking

## RCKid AVR

- DIFFERENT PIN MAPPING FOR NEW VERSION
- flash the LED when charging & idle, add LED effects
- rumbler effects
- joystick calibration (maybe do this in RPi instead together with the accel?) 
- ensure home button long press cannot be spurious (either in SW, or button with more force?)

## RCKid RPi

- DIFFERENT PIN MAPPING FOR NEW VERSION
- audio player
- game player menu implementation (load/save, etc.)
- torchlight widget
- walkie talkie widget
- heart bar for time spent, track time spent when widgets are in foreground
- photores

- key repeat is too fast
- debounce buttons

## RCKid RPi - SDCard

- ensure config on fresh start from clean sd
- disable serial port
- would it make sense to use fewer cores at higher speed? 
- turn off/stuff - https://github.com/raspberrypi/firmware/blob/13691cee95902d76bc88a3f658abeb37b3c90b03/boot/overlays/README#L1335, map to one of the UART pins 
- can we speed things up with crosscompilation? https://deardevices.com/2019/04/18/how-to-crosscompile-raspi/

## LEGO Remote PCB & Case

- add I2C mem for sound effects?

## LEGO Remote AVR

- test the buzzer
- check motor control
- check analog input
- make channel i/o settings when the channel mode changes and not when control value changes
- PWM motor control frequency - 20kHz should be enough
- add RTC PIT for effects
- add INA219

> Non-critical things that would be nice to have, but don't jeopardize the xmas delivery. 

## I2C Bootloader

- cleanup & extend the programmer code so that it can dump the stored files, read/write EEPROM and so on

## RCKid

- walkie & talkie with telegram enabled
- recorder app
- piano app> (but how to do keys?)
- Baby monitor app (MIC threshold wakeup levels on AVR, app on RPi)
- photoresistor

- should button repeat be user-configurable? 
- add == to Value and double parsing
- add JSON ostream << for array and struct
- add JSON backed menu that allows reordering
- how to configure, via JSON I guess,
- samba on demand? etc.
- webserver from the app - see https://github.com/yhirose/cpp-httplib ?
- telegram remote control 

## Raylib 

- the GPU performance is well... bad. 
- check if using VCDispmanX, or SDL can improve things. 
- check that we are find detecting first boot with it


OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD 


# Questions

- AVR chips keep getting bricked... No clue why:(


## PCB & Case


- might also use [this](http://k-silver.com/html_products/JP19%EF%BC%88%E6%AD%A3%E6%8F%92%E8%93%9D%E8%89%B2%E6%91%87%E6%9D%86%EF%BC%89-833.html) thunbstick, that can be bought from [Adafruit](https://www.adafruit.com/product/5628) - will be much easier to solder, might require special pcb board or some such


## RPI - RayLib



- die if in bootloader mode 
- read and reset the error in debug at the beginning of the app

- handle errors in player configs
- actually do the audio, video menu
- on pop for menu so that we can clear it, should we want to
- modal errors & stuff
- add a way for the menu / submenu to detect it's been detached
- add rumbler / sound for ui actions
- if there are errors during startup a way to show them




