# I2C Bootloader

A simple I2C bootloader that fits in 512 bytes.

> https://docs.platformio.org/en/latest/platforms/atmelavr.html#bootloader-programming, https://www.microchip.com/en-us/application-notes/an2634

Note that the UPDI programmer does not check if the chip is correct. If the chip specified in platformio and the actual chip do not match, the memory writes might not work even if everything else would (different memory will be written). 

Both the bootloader and the programmer are pretty specific for the `rcboy`, but minimal changes should make them generic. The bootloader checks the `AVR_IRQ` pin and if 0, 

## Commands

Each I2C command is a single byte, optionally followed by an argument. The commands are designed in such way that they allow page-by-page writing and reading the memory so that the programmer can write and verify the memory contents with minimal AVR code required. 

`0x00` Reserved

> Not used for anything. 

`0x01` RESET

> Resets the chip.

`0x02` WRITE_BUFFER

> Reads bytes from the comms buffer. Reads as many bytes as the client wishes to read wrapping around the buffer size when necessary.  

`0x03 page` WRITE_PAGE

> Writes the contents of the buffer to the specified page and resets the buffer index.

`0x04 page` READ_PAGE

> Reads the specified page into the buffer and resets the buffer index.

`0x05 index` SET_INDEX

> Clears the buffer index, i.e. writing and reading will start from the beginning of the buffer. 

`0x06` INFO

> Fills the buffer with information about the chip, such as fuses and so on.


## Programmer

A simple i2c programmer is provided as well. Usage:

    i2c-programmer write HEX_FILE

`HEX_FILE` is path to the Intel HEX file that should be flashed to the chip. 


