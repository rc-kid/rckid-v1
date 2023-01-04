#include <avr/io.h>

#define I2C_ADDRESS 0x43



namespace i2c {
    struct Command {
        uint8_t cmd;
        uint8_t arg;
    }; 
    
    static void initialize() {
        // turn I2C off in case it was running before
        TWI0.MCTRLA = 0;
        TWI0.SCTRLA = 0;
        // make sure that the pins are nout out - HW issue with the chip, will fail otherwise
        PORTB.OUTCLR = 0x03; // PB0, PB1
        // set the address and disable general call, disable second address and set no address mask (i.e. only the actual address will be responded to)
        TWI0.SADDR = I2C_ADDRESS << 1;
        TWI0.SADDRMASK = 0;
        // enable the TWI in slave mode, enable all interrupts
        TWI0.SCTRLA = TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm  | TWI_ENABLE_bm;
        // bus Error Detection circuitry needs Master enabled to work 
        // not sure why we need it
        TWI0.MCTRLA = TWI_ENABLE_bm;   
    }

    static Command waitForCommand() {

    }
}


/** Upon start the bootloader checks the AVR_IRQ pin. If pulled low, the bootloader will be entered, otherwise the main app code starts immediately. The AVR_IRQ is generally held high by the RPi at 3V3 (which still registers as logical 1) to enable the AVR to signal interrupts so pulling it low works as exceptional case. 
*/
static bool bootloaderRequested() {
    return ! (VPORTA.IN & PIN4_bm);
}

/** The I2C bootloader actual code.
 */
static void bootloader() {
    i2c::initialize();
}

/** Bootloader initialization. 
 
    We bypass the main() and standard files so that w
*/
__attribute__((naked)) __attribute__((constructor)) 
void boot() {
    /* Initialize system for C support */
    asm volatile("clr r1");
    // do we need bootloader? 
    if (bootloaderRequested()) 
        bootloader();
    /* Replace with bootloader code */
    while (1) {
    }
}


