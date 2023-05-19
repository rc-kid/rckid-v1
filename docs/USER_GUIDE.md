> This guide assumes that you have already followed the assembly guide and have a working RCKid at your disposal. 

# RCKid Simple Manual


## Power Up 

To start the device, press and hold the _Home_ button until the rumbler's ok signal (one long rumble), after which the button can be released. Then wait for the device to boot and turn on the screen. 




## Troubleshooting

### Power Up Sequence

After the _Home_ button is pressed, RCKid runs various checks and uses the rumbler and the RGB led to indicate any potential problems. The table bellow summarizes the problems and their visualization: 

RGB Led    | Rumbler   | Notes
-----------|-----------|--------
none       | none      | Normal startup sequence, no errors detected. Wait for the rumbler's ok signal (single long rumble), then relase the _Home_ button and wait couple seconds for the screen brightness to go up. 
3x red     | none      | Critical battery level. RCKid will not start, connect charger to the USB-C port and recharge the battery. 
blue       | none      | Last shutdown timed out, but RPi will attempt to power on normally.
green      | 2x short  | Powerup timeout. The device will power itself off briefly and try to boot again with twice as long interval. If that won't help, it would enter the repair mode and turnoff.
red        | none      | Device starts in repair mode (see below)
red        | 2x short  | Device is in repair mode, but USB power is not connected. Device will not boot. 

## Repair Mode

The _repair mode_ is useful for diagnosing RPi problems as the all timeouts are disabled and the display backlight is set to max by the AVR to ensure visibility. When in repair mode, the device can only power itself when the USB power is connected to prevent excessive battery drainage and spurious power offs. 

The repair mode is entered either manually by holding the _Select_ button while pressing the _Home_ key when turning on, or automatically when the device fails the normal boot as well as its retry. 
