# I2C Bootloader

A simple I2C bootloader. 

## Commands

Each I2C command is a single byte, optionally followed by an argument. The commands are designed in such way that they allow page-by-page writing and reading the memory so that the programmer can write and verify the memory contents with minimal AVR code required. 

`0x00` Reserved


`0x01` WRITE_BUFFER

> Reads bytes from the comms buffer. 

`0x02 PAGE` WRITE_PAGE

> Writes the contents of the buffer to the specified page and resets the buffer index.

`0x03 PAGE` READ_PAGE

> Reads the specified page into the buffer and resets the buffer index.

`0x04` CLEAR_INDEX

> Clears the buffer index, i.e. writing and reading will start from the beginning of the buffer. 

`0x05` RESET

> Resets the chip.

