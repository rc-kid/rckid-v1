> This guide assumes that you have already followed the assembly guide and have a working RCKid at your disposal. 

# RCKid Simple Manual


## Powering Up and Down

To start the device, press and hold the _Home_ button until the rumbler's _ok_ signal (one long rumble), after which the button can be released. Then wait for the device to boot and turn the screen on. 

Press the _Home_ button for a long time again to turn the device off. When turned off, the backlight is disabled and the RPi is given some amount of time to make sure it will turn off safely. 






## Troubleshooting

### Initial Power Up

When power is first applied to the device, the device will power on automatically after a _ok_ rumble signal. The initial power on state will be signalled by a white RGB led. 

### Power Up Sequence

After the _Home_ button is pressed, RCKid runs various checks and uses the rumbler and the RGB led to indicate any potential problems. The table bellow summarizes the problems and their visualization: 

RGB Led    | Rumbler   | Notes
-----------|-----------|--------
none       | none      | Normal startup sequence, no errors detected. 
3x red     | none      | Critical battery level. RCKid will not start, connect charger to the USB-C port and recharge the battery. 
cyan       | none      | AVR Watchdog reset failure, signals that the AVR chip froze. 
blue       | none      | RPi failed to turn itself on within the specified limit during the last run. 
green      | none      | RPi failed to turn itself off withion the specified limit and its power was cutoff abruptly. 

### DebugMode

Press the _Select_ and _Home_ button together to start the device in debug mode when the screen brightness will be powered immediately and the boot timeout will not be set, i.e. RPi can take as long as it wants to boot and the boot process, if any, will be visible on the screen. 
