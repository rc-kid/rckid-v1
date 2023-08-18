> This guide assumes that you have already followed the assembly guide and have a working RCKid at your disposal. 

# RCKid Simple Manual


## Powering Up and Down

To start the device, press and hold the _Home_ button until the rumbler's _ok_ signal (one long rumble), after which the button can be released. Then wait for the device to boot and turn the screen on. 

To power down, navigate the menu to the home section and select the _Power off_ option. This is usually done by pressing the _Home_ button once, or twice when playing a game. 

Alternatively, holding the _Home_ button down util the rumbler's _ok_ signal is given will instruct the RPi to power itself off as well. 

## Charging

RCKid can be charged by any USB-C capable charger. For increased safety, the charging is rather slow. The device can be on while charging. If on, charging status is displayed in the header bar. When off, the RGB led displays charging status, _blue_ light means the device is charging, _green_ light means RCKid is fully charged. 

## Troubleshotting

### Initial Power Up

When power is first applied to the device (be it from attached battery, or USB), the device will power on automatically.  

### Critical Battery

Rumbler's _fail_ signal and 3 bright red LED flashes indicate critical battery level upon power on. Plug the device into USB to to recharge the battery and start again. 

### Power On Failure

If the screen stays black after powering on and in a while the rumbler's _fail_ and red led is displayed, the RPi failed to turn on in time. This could be one-time event, or might require deeper debugging using the forced-on mode. 

### Power Off Failure

If a rumbler _fail_ and red LED is displayed after powering the device off, the RPi failed to signal safe shutdown in time. The device is now off.  

### Forced on mode

Powering the device on with _Home_ and _Select_ buttons both pressed until the _ok_ rumbler signal will enable a forced on mode, in which case the brightness is immediately turned on and all power-on timeouts are disabled so that the state of the device on display can be observed. 
