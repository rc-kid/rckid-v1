> This is the high-level plan on tasks, prioritized, to make the RCKid ready for xmas:

- design the bottom and top frames around the pcb and test print the stack
- add rckid basics (proper header, hearts & configuration and settings)
- print ASA enclosures - ASA samples from here (it actually glows in the dark even): https://www.na3d.cz/p/2621/vzorek-fiber3d-asa-svitici-ve-tme-175-mm-10-m?variant[117]=1071
- order plexiglass covers (https://plasticexpress.cz/)
- test build the assembly
- strip down in-game menu to barebones
- test walkie talkie, add beeps & icon for playing

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

# Things to check

- battery on: 9:30
- charging: 9:45 - 11:22

> Currently in progress/working on. All projects combined. When removing from here make sure to check the sections below for long term items and remove them as needed. 

https://plasticexpress.cz/configurator/1-plexisklo-ir-/2-extrudovan-/3-1-5-mm


- change icon can be done by having all images under one root and that way items can be selected from the pics easily
- rename via keyboard ofc

- allow configurable item action for items (not folders)

- repairing after connection lost does not work (channel & name is different) Add a pairing timeout on the device and wait for pairing indefinitely on the rckid side

> Things to buy / test. 

## Shopping List

- buy some more NRFs (not the cheapest ones)
- https://martybugs.net/wireless/rubberducky.cgi -- site that seems to have decent info on antennas
- better 3.0V LDO (cleaner, higher voltage better for mic & speed, such as https://cz.mouser.com/ProductDetail/Texas-Instruments/TPS7A2030PDBVR?qs=hd1VzrDQEGgZMtQinkbhYw%3D%3D)
- buy extra RPi Zero 2W's (2-3), that way I have parts to build almost 6 RCKids (modulo joysticks)

> TODOs for the various projects that RCkid consists of. Each project has its own todo list.

## RCKid PCB & Case

- make the bottom taller to remove the 0.1mm gap between top and bottom pieces
- make them lock better together, maybe bottom smaller to fit nicely in the case
- can the PCB with joystick soldered on be slided into the case? 
- enlarge the headphone jack a bit
- move USB port a bit higher
- center the light's hole better
- maybe go all the way top/bottom with transparent ribbon for better visibility? 
- is the joystick centered? 

## RCKid AVR

- add LED effects
- rumbler effects

## RCKid RPi

- game player menu implementation (load/save, etc.)
- torchlight widget (?)
- walkie talkie widget
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

- allow games to set the accelerometer or joystick as different buttons

## Raylib 

- the GPU performance is well... bad. 
- check if using VCDispmanX, or SDL can improve things. 
- check that we are find detecting first boot with it

