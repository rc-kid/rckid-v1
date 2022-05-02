# LEGO Remote Controller for Picoboy

The controller consists of ATTiny, radio and the following:

- 2 motor drivers (5 or 9volts)
- 5 extra peripherals

- motor driver: 2x https://www.pololu.com/product/2960
- 9V stepup: 

## Connectors

The connectors are built from headers, in a way that supports only one insertion. 

### Motors

Motors use 8pin connector which is created by 3,3,2 pins in a row. The connector allows the motor to select its preferred voltage. 

5V      VA
9V VSEL VB

### Others

This includes Speaker, RGB lights, buttons... Arranged in 2-1 in a row. 

5V
GND X

> TODO should there be I2C connector? 

## Peripherals

Internally there will be two INA219 for measuring the current on the attached motors. An accelerometer is also a possibility. 

> See if I have enough accels and other useful I2C parts. 

### LEGO 4.5V Motor

Uses the motor connector, 5V input. 

### LEGO Power Functions 9V Motor

Uses the motor connector, 9V input. 

### LEGO NXT Motor 

Uses the motor connector, 9V input. Additionally can use up to 2 peripheral connectors for the speed control. 

- 1 (white) Motor PWR 1
- 2 (black) Motor PWR 2
- 3 (red) GND
- 4 (green) ~4.5V
- 5 (yellow) Encoder 1
- 6 (blue) Encoder 2

### LEGO Compatible servo

Uses the peripheral connector. 

### PWM Speaker

Uses a dedicated peripheral connector that is wired to PWM supporting pin. 

### RGB LEDs (chainable)

Uses a peripheral connector

### Button (chainable)

Uses a peripheral connector. Chaining works as or so that any chained button press will register as press. 