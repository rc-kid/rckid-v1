> This is the high-level plan on tasks, prioritized, to make the RCKid ready for xmas:

- order more displays (3)
- order M1.4 screws for the thumbstick
- test the locking rckid with old revision
- update avr and rckid's for the new revision pins
- solder the new board
- design the bottom and top frames around the pcb and test print the stack
- add AVR basics (charging, thumbstick reporting)
- add rckid basics (proper header, hearts & configuration and settings)
- print ASA enclosures
- order plexiglass covers (https://plasticexpress.cz/)
- test build the assembly
- strip down in-game menu to barebones, add volume controls
- add volume controls to audio & video players
- add walkie-talkie and test it

> At this point, I'll have a barebones, but working rckid that can play some games, play music & video and act as walkie talkie. This can be presented as xmas gift. Then there are two things I can do:

- fix lego remote schematics & pcb layout
- build
- add remote controller to rckid
- build rc car from lego

> or I can focus on improvements to the user stuff, such as:

- reoreding and renaming elements in menus
- icon selector, video & game still captures
- game savepoits
- alarm / other niceties
- telegram configuration
- samba
- wifi settings

> Currently in progress/working on. All projects combined. When removing from here make sure to check the sections below for long term items and remove them as needed. 

https://plasticexpress.cz/configurator/1-plexisklo-ir-/2-extrudovan-/3-1-5-mm

- !!!!!!!! PINS are different, no more volume keys DPAD handled by RPi, L/R not !!!!!!!!

- battery on: 9:30
- charging: 9:45 - 11:22

- (3) order spare displays

- change icon can be done by having all images under one root and that way items can be selected from the pics easily
- rename via keyboard ofc

- allow configurable item action for items (not folders)
- unresponsive-ish buttons on the device in low FPS? (!!!!!!!)


- repairing after connection lost does not work (channel & name is different) Add a pairing timeout on the device and wait for pairing indefinitely on the rckid side

- charging light does not seem to work that well
- make the charging light work even when RPi off / sleeping (check)
- add sin based interleaving (? - would it be enough)
- maybe do just 1sec quick bursts (easier in sleep)
- switch charging control single pullup 

> Things to buy / test. 

## Shopping List

- buy some more NRFs (not the cheapest ones)
- https://martybugs.net/wireless/rubberducky.cgi -- site that seems to have decent info on antennas
- M1.4 screws for thumbstick
- better 3.0V LDO (cleaner, higher voltage better for mic & speed, such as https://cz.mouser.com/ProductDetail/Texas-Instruments/TPS7A2030PDBVR?qs=hd1VzrDQEGgZMtQinkbhYw%3D%3D)
- 5pin 0.5mm pitch connector (https://cz.mouser.com/ProductDetail/Molex/505278-0533?qs=c8NFF48pVsCY0CQNgl3Xjw%3D%3D or https://cz.mouser.com/ProductDetail/Molex/505110-0592?qs=RawsiUxJOFR577kELw3Dww%3D%3D)
- buy extra RPi Zero 2W's (2-3), that way I have parts to build almost 6 RCKids (modulo joysticks)

> TODOs for the various projects that RCkid consists of. Each project has its own todo list.

## RCKid PCB & Case

- make more room for the battery (verify)
- check charging detection works, can perhaps ignore the pullup/downs? into middle and use just one pullup/down? Needs external pull-up. Only works when USB is present - need better USB checking

## RCKid AVR

- DIFFERENT PIN MAPPING FOR NEW VERSION
- flash the LED when charging & idle, add LED effects
- rumbler effects
- joystick calibration (maybe do this in RPi instead together with the accel?) 
- ensure home button long press cannot be spurious (either in SW, or button with more force?)

## RCKid RPi

- DIFFERENT PIN MAPPING FOR NEW VERSION
- game player menu implementation (load/save, etc.)
- torchlight widget
- walkie talkie widget
- heart bar for time spent, track time spent when widgets are in foreground
- photores

## RCKid RPi - SDCard

- ensure config on fresh start from clean sd
- disable serial port
- would it make sense to use fewer cores at higher speed? 
- turn off/stuff - https://github.com/raspberrypi/firmware/blob/13691cee95902d76bc88a3f658abeb37b3c90b03/boot/overlays/README#L1335, map to one of the UART pins 
- can we speed things up with crosscompilation? https://deardevices.com/2019/04/18/how-to-crosscompile-raspi/

## LEGO Remote PCB & Case

- how to do charging (?)

## LEGO Remote AVR

- check motor control
- check analog input
- PWM motor control frequency - 20kHz should be enough
- add INA219 - how will INA be connected to the stuff? - DeviceInfoChannel? 
- remember last pairing information? how to remove it? 
- make batteries into separate brick, makes the design so much simpler I might actually make the xmas deadline

> Non-critical things that would be nice to have, but don't jeopardize the xmas delivery. 

## I2C Bootloader

- cleanup & extend the programmer code so that it can dump the stored files, read/write EEPROM and so on

## RCKid

- walkie & talkie with telegram enabled
- recorder app
- piano app> (but how to do keys?)
- Baby monitor app (MIC threshold wakeup levels on AVR, app on RPi)

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

