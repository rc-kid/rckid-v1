## Assembly

> This repository contains all that is necessary to assemble and program your own _RCKid_. Please note that doing so is likely not a beginner project, at the very least, you will need access to a soldering station and some soldering practice. If there will be any errors during the assembly, multimeter, oscilloscope and some basic electronics knowledge would be required. Some software skills, such as SSH connection and flashing is necessary as well. 

1) File the pcb to ensure that all holes are proper sizes and ensure components do fit.  
2) Solder the accelerometer, components facing down, the bottom side of the accelerometer breakout should be level with the top side of RCKid's pcb. 

![](assembly/2.jpg)

#### USB Power & Battery Charging

3) Solder the USB & battery power bridge (R3, D1, Q2, C27). 

> Verify VCC (via C24 & C25) via either the USB or BATT+ pins (leave the battery connector disconnected yet)

4) Solder the battery charging circuit (D2, C1, C4, IC2, R7, R2, R6). 

#### RPI & RGB Power Switches

5) Add the RPI power switch (R1, Q1)

> Verify VRPI (6) when power applied

6) Add the RGB power switch (R4, R27, Q3)

#### VCLEAN for Audio

7) Add VCLEAN LDO (L1, IC1, C2, C3)

> Check that VCLEAN (30) is 3V

#### ATTiny1616

8) Add ATTiny and its capacitors (ATTINY, C24, C25)
9) Program ATTiny bootloader & fuses via the UPDI (27)

#### 


First we are going to add the `ATTiny1616` and basic `I2C` communication to program the bootloader and verify its presence. To do so, the following must be soldered:

> TODO

When the bootloader is flashed, verify that the `I2C` communication is working by running the AVR programmer in query mode. 

