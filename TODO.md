
- get extra M2.5 inserts
- get 5pin FPC for joysticks

## Rpi image 

- how to install pyupdi directly in platformio penv? so that I do not have to set PATH in tasks

## Rpi - Gamepad & Drivers

- debounce buttons
- can use USB? Host/master?
- shared memory comms with other processes (dim display, battery level, temp, etc.)
- control display with PWM

- only use two axes for the accelerometer, figure out how to make gyro and accel to determine the propoer position

- joystick seems to work, but the responsivity is not the best and it seems some events are missed, check once not on breadboard I guess

# RPi - UI

- TODO

# AVR 

- check it can turn rpi on/off
- check rpi can program it, should there be a way to force RPI on via some button too to override the shutdown? 

# Radio

! Determine which radio to use!

- nrf seems to work both ACK and non ACK with green modules
- check range

## NRF24L01+

- ACKed packets does not seem to work yet

- check ACK payload being delivered

- there's an issue with SPI, the breadboard is pretty unstable affair. Other than that seems to work. Antenna is necessary. I think the rubber duckie can be stripped and added in the case easily. The connector can be semi-exposed to allow for longer range antennas for the walkie-talkie, if ever necessary. Range must be properly tested. 

> https://martybugs.net/wireless/rubberducky.cgi -- site that seems to have decent info on antennas

## SX1278

- test with known library first
