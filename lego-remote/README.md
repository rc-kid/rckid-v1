# LEGO Remote Controller for Picoboy

## Pairing

The control brick is very dumb and does not have any internal state (apart from its name) that would survive power cycle, all is managed by the remote. The handshake looks like this:

                              Controller | Remote
                         --------------- | --------------
                                         | L: (listen for new devices - optional) 
    B: broadcast its name on set channel | 
                                         | P1: send pairing info on set channel
                                         | (own name & channel, wait for ACK)
                                         |
            switch channel & wait for P2 | switch to channel, send P2, wait for ACK
                                         |
         (timeout 1 second, return to B) | (timeout 1 second, return to L, or P1)

While pairing the controller flashes red lights. When paired, blue is flashed. Upon pairing the remote uploads the connector profile, after which the pairing is complete. 

While paired the remote periodically updates the controller every 1/100th of a second (10ms). If there is no update for 50ms the controller enters the disconnected state, in which case all motors are powered off and lights flash red. Upon regaining connection, the remote must acknowledge the disconnect before the motors can be powered on again. 

## Power


The controller brick operates from any voltage between 5 and 9 volts, inclusive. It uses a special connector that supports connecting 2 Li-Ion cells in series as well as other configurations:

    1N    2P  
    1N 2N 2P
    1P 1P 2N

Internally the `1P` and `2N` terminals are connected together to provide 7.4V between `1N` and `2P` which should be used for other sources. 

The controller consists of ATTiny, radio and the following:

- 2 motor drivers (5 or 9volts)
- 5 extra peripherals

- motor driver: 2x https://www.pololu.com/product/2960
- 9V stepup: 

## Connectors

The connectors are built from headers, in a way that supports all possible orientations by doubling the pins. 

### Motors

Motors use 3x4 12pin connector that allows the motor to select its own voltage as well as draw voltage for electronics. 

    5V   +   -   9V
    SEL GND GND SEL
    9V   -   +   5V

### I/O


This includes Speaker, RGB lights, buttons... Arranged in 2-1 in a row. 

    5V    X
    GND GND
    X    5V

### Expander

The expander connector is a 7pin 3x2x3 connector with the following layout:

    SDA     SCL
    5V  GND  5V 
    SCL     SDA

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

Uses the peripheral connector in servo mode. 

### PWM Speaker

Uses a dedicated peripheral connector that is wired to PWM supporting pin. 

### RGB LEDs (chainable)

Uses a peripheral connector

### Button (chainable)

Uses a peripheral connector. Chaining works as or so that any chained button press will register as press. 