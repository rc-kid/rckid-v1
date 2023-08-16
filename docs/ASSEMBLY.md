# Assembly

Please note that building _RCKid_ is likely not a beginner project. At the very least, you will need access to a soldering station and some soldering practice. If there will be any errors during the assembly, multimeter, oscilloscope and some basic electronics knowledge would be required. Some software skills, such as SSH connection and flashing is necessary as well. 

## PCB

Have the PCB printed. Simple 2 layer stackup is used with relatively large tolerances. Use either the [diptrace project](../hardware/rckid.dip) itself, or the [gerber files](../hardware/rckid.zip) that are part of this repository. The board thickness is expected to be 1.6mm. 

> I do recommend [aisler](https://aisler.net) if your are in Europe. 

Break out the small left and right trigger button PCBs located at the top of the board, sand any rough edges from mousebites and ensure the joystick fits through the joystick hole, sanding the round corners from milling to right angles. 

### Accelerometer

> Accelerometer is optional. 

Sand the rounded edges of the accelerometer hole to right angles to ensure accelerometer fit. Then press the accelerometer in with its components on the bottom side. Ensure the bottom side of the accelerometer pcb lines up perfectly with the top side of the RCKid pcb. Then cover from the top side with kapton tape, flip RCKid pcb so that you can see the bottom side (with accel components now visible) and solder the accelerometer in the castellated holes provided on both sides. 

### Top-side PCB components

Flip the RCKid so that the top side is visible (with logo & display connector) and solder all items except the display, microphone module and the 3V3 module for the radio. 

### 3V3 Buck Converter

Trip the module to the required size by making the pin holes into castellations and cutting the top piece roughly in the middle of the mounting hole. Then solder to place. 

### Basic tests

Do the following basic tests to ensure the board is in good condtition:

- verify that GND and VCC / VBATT is not shorted
- connect 5V power to the USB-C breakout pins (the breakout itself is not yet soldered) and verify the voltages at RPi, AVR, NRF radio and the 3V clean LDO for microphone and audio

### AVR Bootloader

Solder the downward facing precission sockets for the GND and UPDI programming and then program the AVR's bootloader. Make sure to connect the GND first (!). 

> You can use either a dedicated UPDI programmer (a detailed description of how to make one can be found [here]())  
























## Soldering PCB 


## Instructions

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

## Powering Up

To power up attach the (ideally pre-charged) battery and optionally the USB-C charger as well. Upon power-on the AVR chip enters the power-on sequence