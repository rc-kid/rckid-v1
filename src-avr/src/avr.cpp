#include <Arduino.h>

#include "platform/platform.h"
#include "platform/gpio.h"
#include "platform/i2c.h"

#include "comms.h"

/** Chip Pinout
               -- VDD             GND --
               -- (00) PA4   PA3 (16) -- 
               -- (01) PA5   PA2 (15) -- 
               -- (02) PA6   PA1 (14) -- 
               -- (03) PA7   PA0 (17) -- UPDI
               -- (04) PB5   PC3 (13) -- 
               -- (05) PB4   PC2 (12) -- 
               -- (06) PB3   PC1 (11) -- DEBUG
               -- (07) PB2   PC0 (10) -- IRQ
     SDA (I2C) -- (08) PB1   PB0 (09) -- SCL (I2C)

    The AVR is responsible for monitoring the physical inputs and power management. It communicates with RP2040 via I2C and an IRQ pin that AVR rises when there is an input change. 
 */

/* Pins Available: 15

   - 3v3en for pico
   - irq for pico
   - X and Y analog
   - A B C D buttons
   - L R buttons
   - PWR button
   - headphones 
   - battery voltage (?)
   - 2

 */


static constexpr gpio::Pin DEBUG_PIN = 11;
static constexpr gpio::Pin IRQ_PIN = 10;

struct {
    State state;
    uint8_t i2cBuffer[I2C_BUFFER_SIZE];
    uint8_t i2cBytesRecvd = 0;
    uint8_t i2cBytesSent = 0;
} comms;

volatile struct {
    bool i2cReady : 1;
    bool secondTick : 1;
    bool sleep : 1;
} flags;

void setup() {
    gpio::output(DEBUG_PIN);
    gpio::low(DEBUG_PIN);

    gpio::input(IRQ_PIN);

    // initialize the RTC to fire every second
    RTC.CLKSEL = RTC_CLKSEL_INT1K_gc; // select internal oscillator divided by 32
    RTC.PITINTCTRL |= RTC_PI_bm; // enable the interrupt
    while (RTC.PITSTATUS & RTC_CTRLBUSY_bm) {}
    RTC.PITCTRLA = RTC_PERIOD_CYC1024_gc + RTC_PITEN_bm;
    // initailize ADC0 responsible for temperature, voltages and peripherals
    // voltage reference to 1.1V (internal)
    VREF.CTRLA &= ~ VREF_ADC0REFSEL_gm;
    VREF.CTRLA |= VREF_ADC0REFSEL_1V1_gc;
    // delay 32us and sampctrl of 32 us for the temperature sensor, do averaging over 64 values, full precission
    ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc;
    ADC0.CTRLB = ADC_SAMPNUM_ACC64_gc;
    ADC0.MUXPOS = ADC_MUXPOS_INTREF_gc;
    ADC0.CTRLC = ADC_PRESC_DIV16_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm; // use VDD as reference for VCC sensing
    ADC0.CTRLD = ADC_INITDLY_DLY32_gc;
    ADC0.SAMPCTRL = 31;
    // initialize ADC1 to sample the X-Y thumbstick position
    // delay & sample averaging for increased precision
    ADC1.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_8BIT_gc;
    ADC1.CTRLB = ADC_SAMPNUM_ACC64_gc;
    ADC1.MUXPOS = ADC_MUXPOS_TEMPSENSE_gc;
    ADC1.CTRLC = ADC_PRESC_DIV16_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm; // use VDD as reference 
    ADC1.CTRLD = ADC_INITDLY_DLY32_gc;
    ADC1.SAMPCTRL = 31;

    // start conversions on the ADCs
    ADC0.COMMAND = ADC_STCONV_bm;
    //ADC1.COMMAND = ADC_STCONV_bm;






    i2c::initializeSlave(AVR_I2C_ADDRESS);

}

void processCommand() {
    // TODO

    comms.i2cBytesRecvd = 0;
    cli();
    flags.i2cReady = false;
    sei();
}

void processADC0Result() {
    uint16_t value = ADC0.RES / 64;
    switch (ADC0.MUXPOS) {
        // convert value to voltage * 100, using the x = 1024 / value * 110 equation. Multiply by 512 only, then divide and multiply by two so that we always stay within uint16_t
        case ADC_MUXPOS_INTREF_gc: {
            value = 110 * 512 / value;
            value = value * 2;
            cli();
            comms.state.setVcc(value);
            sei();
            ADC0.MUXPOS = ADC_MUXPOS_TEMPSENSE_gc;
            ADC0.CTRLC = ADC_PRESC_DIV16_gc | ADC_REFSEL_INTREF_gc | ADC_SAMPCAP_bm; // internal reference
            break;
        }
        // convert the temperature to temp * 10 with 0.5C precission
        case ADC_MUXPOS_TEMPSENSE_gc: {
            // taken from the ATTiny datasheet example
            int8_t sigrow_offset = SIGROW.TEMPSENSE1; 
            uint8_t sigrow_gain = SIGROW.TEMPSENSE0;
            int32_t temp = value - sigrow_offset; // Result might overflow 16 bit variable (10bit+8bit)
            temp *= sigrow_gain;
            // temp is now in kelvin range, to convert to celsius, remove -273.15 (x256)
            temp -= 69926;
            // and now loose precision to 0.5C (x10, i.e. -15 = -1.5C)
            temp = (temp >>= 7) * 5;
            cli();
            comms.state.setTemp(temp);
            sei();
            // TODO deal with sensing battery voltage here
            // fallthrough
        }
        default:
            ADC0.MUXPOS = ADC_MUXPOS_INTREF_gc;
            ADC0.CTRLC = ADC_PRESC_DIV16_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm; // use VDD as reference for VCC sensing
            break;
    }
    // start new conversion
    ADC0.COMMAND = ADC_STCONV_bm;
}

void processADC1Result() {
    uint8_t value = ADC1.RES / 64;
    switch (ADC1.MUXPOS) {

    }
    // start new conversion
    ADC1.COMMAND = ADC_STCONV_bm;
}


void loop() {
    if (flags.i2cReady)
        processCommand();
    if (ADC0.INTFLAGS & ADC_RESRDY_bm)
        processADC0Result();
    if (ADC1.INTFLAGS & ADC_RESRDY_bm)
        processADC1Result();
}

/** RTC ISR
 
    Simply increases the time by one second.
 */
ISR(RTC_PIT_vect) {
    gpio::high(DEBUG_PIN);
    RTC.PITINTFLAGS = RTC_PI_bm; // clear the interrupt
    comms.state.time().secondTick();
    flags.secondTick = true;
    gpio::low(DEBUG_PIN);
}

/** Handling of I2C Requests
 */
ISR(TWI0_TWIS_vect) {
    #define I2C_DATA_MASK (TWI_DIF_bm | TWI_DIR_bm) 
    #define I2C_DATA_TX (TWI_DIF_bm | TWI_DIR_bm)
    #define I2C_DATA_RX (TWI_DIF_bm)
    #define I2C_START_MASK (TWI_APIF_bm | TWI_AP_bm | TWI_DIR_bm)
    #define I2C_START_TX (TWI_APIF_bm | TWI_AP_bm | TWI_DIR_bm)
    #define I2C_START_RX (TWI_APIF_bm | TWI_AP_bm)
    #define I2C_STOP_MASK (TWI_APIF_bm | TWI_DIR_bm)
    #define I2C_STOP_TX (TWI_APIF_bm | TWI_DIR_bm)
    #define I2C_STOP_RX (TWI_APIF_bm)
    //gpio::high(DEBUG_PIN);
    uint8_t status = TWI0.SSTATUS;
    // sending data to accepting master is on our fastpath as is checked first, if there is more state to send, send next byte, otherwise go to transcaction completed mode. 
    if ((status & I2C_DATA_MASK) == I2C_DATA_TX) {
        if (comms.i2cBytesSent < sizeof (comms.state) + sizeof (comms.i2cBuffer)) {
            TWI0.SDATA = reinterpret_cast<uint8_t*>(& comms.state)[comms.i2cBytesSent++];
            TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
        } else {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
        }
    // a byte has been received from master. Store it and send either ACK if we can store more, or NACK if we can't store more
    } else if ((status & I2C_DATA_MASK) == I2C_DATA_RX) {
        comms.i2cBuffer[comms.i2cBytesRecvd++] = TWI0.SDATA;
        TWI0.SCTRLB = (comms.i2cBytesRecvd == sizeof(comms.i2cBuffer)) ? TWI_SCMD_COMPTRANS_gc : TWI_SCMD_RESPONSE_gc;
    // master requests slave to write data, clear the IRQ and prepare to send the state
    } else if ((status & I2C_START_MASK) == I2C_START_TX) {
        gpio::input(IRQ_PIN);
        comms.i2cBytesSent = 0;
        TWI0.SCTRLB = TWI_ACKACT_ACK_gc + TWI_SCMD_RESPONSE_gc;
    // master requests to write data itself. ACK if there is no pending I2C message, NACK otherwise
    } else if ((status & I2C_START_MASK) == I2C_START_RX) {
        TWI0.SCTRLB = flags.i2cReady ? TWI_ACKACT_NACK_gc : TWI_SCMD_RESPONSE_gc;
    // sending finished
    } else if ((status & I2C_STOP_MASK) == I2C_STOP_TX) {
        TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
    // receiving finished, inform main loop we have message waiting
    } else if ((status & I2C_STOP_MASK) == I2C_STOP_RX) {
        TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
        flags.i2cReady = true;
    } else {
        // error - a state we do not know how to handle
    }
    //gpio::low(DEBUG_PIN);
}
