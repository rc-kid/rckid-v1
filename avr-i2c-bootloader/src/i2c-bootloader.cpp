#include <avr/io.h>

#include "config.h"

/**
                   -- VDD             GND --
           AVR_IRQ -- (00) PA4   PA3 (16) -- 
         BACKLIGHT -- (01) PA5   PA2 (15) -- 
                   -- (02) PA6   PA1 (14) -- 
                   -- (03) PA7   PA0 (17) -- UPDI
                   -- (04) PB5   PC3 (13) -- 
                   -- (05) PB4   PC2 (12) -- 
                   -- (06) PB3   PC1 (11) -- 
                   -- (07) PB2   PC0 (10) -- 
         SDA (I2C) -- (08) PB1   PB0 (09) -- SCL (I2C)

*/

#define I2C_DATA_MASK (TWI_DIF_bm | TWI_DIR_bm) 
#define I2C_DATA_TX (TWI_DIF_bm | TWI_DIR_bm)
#define I2C_DATA_RX (TWI_DIF_bm)
#define I2C_START_MASK (TWI_APIF_bm | TWI_AP_bm | TWI_DIR_bm)
#define I2C_START_TX (TWI_APIF_bm | TWI_AP_bm | TWI_DIR_bm)
#define I2C_START_RX (TWI_APIF_bm | TWI_AP_bm)
#define I2C_STOP_MASK (TWI_APIF_bm | TWI_DIR_bm)
#define I2C_STOP_TX (TWI_APIF_bm | TWI_DIR_bm)
#define I2C_STOP_RX (TWI_APIF_bm)

/* Define application pointer type */
typedef void (*const app_t)(void);

uint8_t command;
uint8_t arg;
uint8_t i;
uint8_t buffer[MAPPED_PROGMEM_PAGE_SIZE];

/** Bootloader initialization. 
 
    We bypass the main() and standard files so that w
*/
__attribute__((naked)) __attribute__((constructor)) 
void boot() {
    /* Initialize system for C support */
    asm volatile("clr r1");
    // ensure that when PA4 (AVR_IRQ) is switched to output mode, the pin is pulled low 
    VPORTA.OUT &= ~ (1 << 4); 
    // only enter the bootloader if PA4 (AVR_IRQ) is pulled low
    if ((VPORTA.IN & PIN4_bm) == 0) {
        // enable the display backlight when entering the bootloader for some output (like say, eventually an OTA:) Baclight is connected to PA5 and is active high
        VPORTA.DIR |= (1 << 5);
        VPORTA.OUT |= (1 << 5);
        // initialize the I2C in slave mode w/o interrupts
        // turn I2C off in case it was running before
        TWI0.MCTRLA = 0;
        TWI0.SCTRLA = 0;
        // make sure that the pins are not out - HW issue with the chip, will fail otherwise
        PORTB.OUTCLR = 0x03; // PB0, PB1
        // set the address and disable general call, disable second address and set no address mask (i.e. only the actual address will be responded to)
        TWI0.SADDR = I2C_ADDRESS << 1;
        TWI0.SADDRMASK = 0;
        // enable the TWI in slave mode, enable all interrupts
        TWI0.SCTRLA = TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm  | TWI_ENABLE_bm;
        // bus Error Detection circuitry needs Master enabled to work 
        TWI0.MCTRLA = TWI_ENABLE_bm;   
        // start the main loop that processes the commands
        while (true) {
            uint8_t status = TWI0.SSTATUS;
            // sending data to accepting master is on our fastpath as is checked first, the next byte in the buffer is sent, wrapping the index on 256 bytes 
            if ((status & I2C_DATA_MASK) == I2C_DATA_TX) {
                TWI0.SDATA = buffer[i];
                i = (i + 1) % MAPPED_PROGMEM_PAGE_SIZE;
                TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
            // a byte has been received from master. Store it and send either ACK if we can store more, or NACK if we can't store more
            } else if ((status & I2C_DATA_MASK) == I2C_DATA_RX) {
                switch (command) {
                    case CMD_RESERVED:
                        command = TWI0.SDATA;
                        break;
                    case CMD_WRITE_BUFFER:
                        buffer[i] = TWI0.SDATA;
                        i = (i + 1) % MAPPED_PROGMEM_PAGE_SIZE;
                        break;
                    default:
                        arg = TWI0.SDATA;
                        break;
                }
                TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
            // master requests slave to write data, simply say we are ready as we always send the buffer
            } else if ((status & I2C_START_MASK) == I2C_START_TX) {
                TWI0.SCTRLB = TWI_ACKACT_ACK_gc + TWI_SCMD_RESPONSE_gc;
            // master requests to write data itself, first the command code
            // pull the AVR_IRQ low to signal we are busy with the command by enabling output on PA4
            } else if ((status & I2C_START_MASK) == I2C_START_RX) {
                command = CMD_RESERVED; // command will be filled in 
                TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
                VPORTA.DIR |= (1 << 4); 
            // sending finished, there is nothing to do 
            } else if ((status & I2C_STOP_MASK) == I2C_STOP_TX) {
                TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
            // receiving finished, if we are in command mode, process the command, otherwise there is nothing to be done 
            } else if ((status & I2C_STOP_MASK) == I2C_STOP_RX) {
                TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
                switch (command) {
                    // Reset the AVR - first signal the command is processed, then reset the chip
                    case CMD_RESET:
                        VPORTA.DIR &= ~(1 << 4);
                        _PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRE_bm);
                    case CMD_INFO:
                        buffer[0] = SIGROW.DEVICEID0;
                        buffer[1] = SIGROW.DEVICEID1;
                        buffer[2] = SIGROW.DEVICEID2;
                        buffer[3] = 0; // bootloader
                        for (uint8_t i = 0; i < 10; ++i)
                            buffer[4 + i] = ((uint8_t*)(&FUSE))[i];
                        buffer[15] = CLKCTRL.MCLKCTRLA;
                        buffer[16] = CLKCTRL.MCLKCTRLB;
                        buffer[17] = CLKCTRL.MCLKLOCK;
                        buffer[18] = CLKCTRL.MCLKSTATUS;
                        buffer[19] = MAPPED_PROGMEM_PAGE_SIZE >> 8;
                        buffer[20] = MAPPED_PROGMEM_PAGE_SIZE & 0xff;
                        // reset the buffer
                        i = 0;
                        //buffer[11] = FUSE.LOCKBIT;
                        break;
                    case CMD_WRITE_PAGE: {
                        uint8_t * page = (uint8_t*)(MAPPED_PROGMEM_START + arg * MAPPED_PROGMEM_PAGE_SIZE);
                        for (uint8_t i = 0; i < MAPPED_PROGMEM_PAGE_SIZE; ++i)
                            page[i] = buffer[i];
                        _PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, NVMCTRL_CMD_PAGEERASEWRITE_gc);
                        while(NVMCTRL.STATUS & NVMCTRL_FBUSY_bm);                
                        break;
                    }
                    case CMD_READ_PAGE: {
                        uint8_t * page = (uint8_t*)(MAPPED_PROGMEM_START + arg * MAPPED_PROGMEM_PAGE_SIZE);
                        for (uint8_t i = 0; i < MAPPED_PROGMEM_PAGE_SIZE; ++i)
                            buffer[i] = page[i];
                        break;
                    }
                    case CMD_SET_INDEX:
                        i = arg;
                        break;
                    default:
                        // nothing to be done for the rest
                        break;
                }
                // switch AVR_IRQ (PA4) to input again
                VPORTA.DIR &= ~(1 << 4);
            // nothing to do, or error - not sure what to do, perhaps reset the bootloader? 
            } else {
                // TODO
            }
        }
    }
    // enable the boot lock so that app can't override the bootloader and then start the app
    NVMCTRL.CTRLB = NVMCTRL_BOOTLOCK_bm;
    // start the application
    app_t app = (app_t)(BOOTLOADER_SIZE / sizeof(app_t));
    app();    
}
