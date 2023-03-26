# RCKid Features

> A draft of the features of the system. Work in progress, very rough. 


## Powering On

Long press of the _Home_ button initiates the power-on sequence of the device. Press the _Home_ button down and hold it until the device rumbles for start. After the button is pressed, RCKid runs internal diagnostics and the following outcomes, signalled by the rumbler and/or the RGB are possible:

RGB     | Rumbler | Description
--------|---------|-------------
none    | none    | Normal startup sequence, no errors detected. Wait for the rumbler's ok signal to release the _home_ button. 
3 x red | none    | Critical battery level. RCKid will not start, connect charger to the USB-C port and recharge the battery.
blue    | none    | Attempting to power on normally, but last power down timed out. 
green   | none    | Immediate power on retry after the RPi failed to boot in time previous. 2 retries will be attempted with progressively longer boot timeouts. If all are unsuccessful, the device enters the repair mode. 
1x red  | fail    | Device is in repair mode, but not connected to the charger. Connect to the charger and start again to start RCKid in repair mode. 
red     | none    | Device starts in repair mode. Must be connected to the charger. 

## Repair Mode

The repair mode is activated when the AVR is unable to wake up the Rpi and therefore RCKid cannot resume normal operation. During the repair mode the timeouts are disabled and the RPi is constantly powered so that debugging it can be enabled. 







## Controls

- when either of the volume buttons are pressed, they control the volume (with repeat mode on)
- when both are pressed we enter the admin menu. This gives the delay too

## Main Menu

- play games from retroarch
- play music stored in playlists
- play videos stored in playlists
- walkie-talkie
- remote control

## Control Menu

Control menu is reached by long pressing volume up and volume down simultaneously. It consists of the following items:

- power off
- airplane mode
- torchlight
- baby monitor
- settings

## Settings

- brightness (then a progress bar) 
- volume (then a progress bar)
- wifi (then selection of networks to try to connect to + own AP)
- informaton (extra info about wifi, connecting, etc.)

When in the control menu, the header displays the details (battery capacity in pct, connected wifi network, etc.)

## Playging Games

Various libretro cores and games can be played. Optimized for kids, i.e. you select games, not cores. 

> Should games be organized in playlists?

## Playing Music

Ideally do this from within the app so that we can keep control of the screen. Organized in playlists. 

## Playing Video

Also ideally play from within the app so that we can regain control. Organized in playlists. 

## Walkie Talkie

A simple push to talk radio using the nrf chip onboard.

> If there is no-one within reach, the radio can switch to telegram mode and start pushing to telegram if wifi is enabled?

## Remote Control

A remote control app that uses the NRF radio. Different profiles may be chosen and configured from within the app. 

> Need to determine the API for the remote control, feedback, etc. 

## Torchlight

Simple torchlight with extra functions, such s blinking, colors, etc. 

> Is this part of the main menu, or the advanced menu with power off? 

# Extra features

- allow time budgets for different activities? Allow topping up the budgets by parents and so on? 
- tamagotchi? 
- web server for more advanced settings & control by parents? 
- customizable easter eggs for the kids? 

# Very extra features

- libretro multiplayer via nrf or some other means

