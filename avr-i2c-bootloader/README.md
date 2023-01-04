# I2C Bootloader

A simple I2C bootloader. 

## Commands

Each I2C command is a single byte, optionally followed by an argument. The commands are designed in such way that they allow page-by-page writing and reading the memory so that the programmer can write and verify the memory contents with minimal AVR code required. 

`0x00` TEST

> Verifies the bootloader is working by returning a magic number `0xCAFEBABE`

`0x01` WRITE_BUFFER

> Expects 32 bytes that will be appended to the comms buffer. 

`0x02` READ_BUFFER

> Reads next 32 bytes from the comms buffer. 

`0x03 PAGE` WRITE_PAGE

> Writes the contents of the buffer to the specified page and resets the buffer index.

`0x04 PAGE` READ_PAGE

> Reads the specified page into the buffer and resets the buffer index.

`0x05` CLEAR_INDEX

> Clears the buffer index, i.e. writing and reading will start from the beginning of the buffer. 

`0x06` RESET

> Resets the chip.

