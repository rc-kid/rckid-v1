#include <Arduino.h>

#include "platform/platform.h"

#include "comms.h"

/** Chip pinout and considerations

                   -- VDD             GND --
           AVR_IRQ -- (00) PA4   PA3 (16) -- VIB_EN
         BACKLIGHT -- (01) PA5   PA2 (15) -- CHARGE
            RGB_EN -- (02) PA6   PA1 (14) -- BTN_LVOL
             VBATT -- (03) PA7   PA0 (17) -- UPDI
          PHOTORES -- (04) PB5   PC3 (13) -- RGB
           MIC_OUT -- (05) PB4   PC2 (12) -- JOY_V
          BTN_RVOL -- (06) PB3   PC1 (11) -- JOY_BTN
            RPI_EN -- (07) PB2   PC0 (10) -- JOY_H
         SDA (I2C) -- (08) PB1   PB0 (09) -- SCL (I2C)

    Powering on 

    The AVR is in sleep mode, registering changes on the L and R volume buttons and waking up every second via the RTC timer. When either button is pressed, AVR switches to standby mode, enables the tick timer at 10ms and attempts to determine what 
*/

/** Enables or disables power for the rpi, audioo and most other sensors including buttons, accelermeter and photoresistor. When left floating (default), the power is on so that the rpi can be used to program the AVR without loss of power. To turn the rpi off, the pin must be driven high. 
 
    Connected to a 100k pull-down resistor this means that when rpi is turned off, we need 0.05mA. 
 */
static constexpr gpio::Pin RPI_EN = 7;

/** Communication with the rpi is done via I2C and an IRQ pin. The rpi is the sole master of the i2c bus. When avr wants to signal rpi a need to communicate, it drives the AVR_IRQ pin low. Otherwise the pin should be left floating as it is pulled up by the rpi. 

TODO can this be used to check rpi is off?  
 */
static constexpr gpio::Pin I2C_SDA = 8;
static constexpr gpio::Pin I2C_SCL = 9;
static constexpr gpio::Pin AVR_IRQ = 0;

/** The backlight and vibration motor can be PWM signals using the TCB timer. 
 */
static constexpr gpio::Pin BACKLIGHT = 1; // TCB0
static constexpr gpio::Pin VIB_EN = 16; // TCB1

/** The RGB light can be turned off as the LED consumes quite a lot of power (mA) even when idle. Once turned on the RGB pin can be used to signal the color. The RGB power is independent of the RPI power and can therefore be used even when the rpi is off (such as flashlight, or low battery warning).
 */ 
static constexpr gpio::Pin RGB_EN = 2;
static constexpr gpio::Pin RGB = 13;

/** Left anr right volume buttons which are always available. Their long press enables the device, or turns the flashlight on. 
 */
static constexpr gpio::Pin BTN_RVOL = 6;
static constexpr gpio::Pin BTN_LVOL = 14;

/** Joystick inputs for horizontal and vertical position and the button. Their range is 0-3.3V and therefore are analog inputs. They are powered by the audio 3V3 line and so are only working the rpi is on. 
 */ 
static constexpr gpio::Pin JOY_H = 10; // ADC1(6)
static constexpr gpio::Pin JOY_BTN = 11; // ADC1(7)
static constexpr gpio::Pin JOY_V = 12; // ADC1(8)

/** Microphone input. The microphone is on when rpi is on. 
 */
static constexpr gpio::Pin MIC_OUT = 5; // ADC0(9)

/** Photoresistor input. 0-3v3 range. On when rpi is on. 
 */
static constexpr gpio::Pin PHOTORES = 4; // ADC0(8)

/** Battery voltage input. Can be used to get the battery voltage regardless of charging/usb power status. 
 */
static constexpr gpio::Pin VBATT = 3; // ADC0(7)

/** Determines the state of the battery charger. High means the battery charging has finished. Low means the battery is being charged. There is also a high impedance mode, which is shutdown or no battery which we do not use. In those states the reading of the pin can be waird.
 
    TODO Check it's ok for the pin to be digital. 
 */
static constexpr gpio::Pin CHARGE = 15; // ADC0(2)






static constexpr uint8_t NUM_BUTTONS = 2;
static constexpr uint8_t BUTTON_DEBOUNCE = 10; // [ms]
static constexpr uint16_t POWERON_PRESS = 1000; // [ms]

struct {
    comms::State state;
    uint8_t i2cBuffer[comms::I2C_BUFFER_SIZE];
    uint8_t i2cBytesRecvd = 0;
    uint8_t i2cBytesSent = 0;
} communication;

volatile struct {
    bool irq : 1;
    bool i2cReady : 1;
    bool tick : 1; 
    bool secondTick : 1;
    bool sleep : 1;
} flags;

/** Timers for the button debounce. 
 */
uint8_t buttonTimers[NUM_BUTTONS] = {0,0};

void wakeup();
void pwrButtonDown();




/** \name Clocks
 
    Simply increases the time by one second and sets the second tick flag so that the main loop can react to the second increase in due time. 
 */
namespace clocks {

    /** Initializes the RTC and starts its IRQ with 1 second interval. 
     */
    void startRTC() {
        // initialize the RTC to fire every second
        RTC.CLKSEL = RTC_CLKSEL_INT1K_gc; // select internal oscillator divided by 32
        RTC.PITINTCTRL |= RTC_PI_bm; // enable the interrupt
        RTC.PITCTRLA = RTC_PERIOD_CYC1024_gc + RTC_PITEN_bm;
    }

    /** Starts the clock tick with 100Hz interval, which is used for updating the user controls, rpi notifications and general effects. 
     */
    void startTick() {

    }

    void stopTick() {

    }
}

ISR(RTC_PIT_vect) {
    RTC.PITINTFLAGS = RTC_PI_bm; // clear the interrupt
    communication.state.time().secondTick();
    flags.secondTick = true;
}

/** \name Interface with the RPi.
 
    Communication is done via I2C and the IRQ pin that is pulled up by RPi and can be driven low by AVR if it wants to communicate a state change to RPi. 
 */
namespace rpi {

    __inline__ void on() {
        gpio::input(RPI_EN);
    }

    __inline__ void off() {
        gpio::output(RPI_EN);
        gpio::high(RPI_EN);
    }

    void processCommand() {
        // TODO

        // clear the received bytes buffer and the i2c command ready flag
        communication.i2cBytesRecvd = 0;
        cli();
        flags.i2cReady = false;
        sei();
    }
}

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
    uint8_t status = TWI0.SSTATUS;
    // sending data to accepting master is on our fastpath as is checked first, if there is more state to send, send next byte, otherwise go to transcaction completed mode. 
    if ((status & I2C_DATA_MASK) == I2C_DATA_TX) {
        if (communication.i2cBytesSent < sizeof (communication.state) + sizeof (communication.i2cBuffer)) {
            TWI0.SDATA = reinterpret_cast<uint8_t*>(& communication.state)[communication.i2cBytesSent++];
            TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
        } else {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
        }
    // a byte has been received from master. Store it and send either ACK if we can store more, or NACK if we can't store more
    } else if ((status & I2C_DATA_MASK) == I2C_DATA_RX) {
        communication.i2cBuffer[communication.i2cBytesRecvd++] = TWI0.SDATA;
        TWI0.SCTRLB = (communication.i2cBytesRecvd == sizeof(communication.i2cBuffer)) ? TWI_SCMD_COMPTRANS_gc : TWI_SCMD_RESPONSE_gc;
    // master requests slave to write data, clear the IRQ and prepare to send the state
    } else if ((status & I2C_START_MASK) == I2C_START_TX) {
        gpio::input(AVR_IRQ);
        communication.i2cBytesSent = 0;
        TWI0.SCTRLB = TWI_ACKACT_ACK_gc + TWI_SCMD_RESPONSE_gc;
        //gpio::input(IRQ_PIN); // clear IRQ
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
}

/** \name ADC0
 
    ADC0 is used both for power management and for microphone reading - for more information about the microphone reading see the Audio section. 
 */
namespace adc0 {

    static constexpr uint8_t MUXPOS_VCC = ADC_MUXPOS_INTREF_gc;
    static constexpr uint8_t MUXPOS_VBATT = ADC_MUXPOS_AIN7_gc;
    static constexpr uint8_t MUXPOS_PHOTORES = ADC_MUXPOS_AIN8_gc;
    static constexpr uint8_t MUXPOS_MIC = ADC_MUXPOS_AIN9_gc;
    static constexpr uint8_t MUXPOS_TEMP = ADC_MUXPOS_TEMPSENSE_gc; // uses 1V1 internal reference

    uint16_t accPhotores_;
    uint8_t accPhotoresSize_;

    void initialize() {
        static_assert(VBATT == 3); // AIN7 - ADC0, PA7
        PORTA.PIN7CTRL &= ~PORT_ISC_gm;
        PORTA.PIN7CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTA.PIN7CTRL &= ~PORT_PULLUPEN_bm;
        static_assert(PHOTORES == 4); // AIN8 - ADC0, PB5
        PORTB.PIN5CTRL &= ~PORT_ISC_gm;
        PORTB.PIN5CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTB.PIN5CTRL &= ~PORT_PULLUPEN_bm;
        static_assert(MIC_OUT == 5); // AIN9 - ADC0, PB4
        PORTB.PIN4CTRL &= ~PORT_ISC_gm;
        PORTB.PIN4CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTB.PIN4CTRL &= ~PORT_PULLUPEN_bm;
    }

    void start() {
        accPhotores_ = 0;
        accPhotoresSize_ = 0;
        // voltage reference to 1.1V (internal for the temperature sensor)
        VREF.CTRLA &= ~ VREF_ADC0REFSEL_gm;
        VREF.CTRLA |= VREF_ADC0REFSEL_1V1_gc;
        // delay 32us and sampctrl of 32 us for the temperature sensor, do averaging over 64 values, full precission
        ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc;
        ADC0.CTRLB = ADC_SAMPNUM_ACC64_gc;
        ADC0.MUXPOS = MUXPOS_VCC; // start by measuring the internal voltage 
        ADC0.CTRLC = ADC_PRESC_DIV16_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm; // use VDD as reference for VCC sensing
        ADC0.CTRLD = ADC_INITDLY_DLY32_gc;
        ADC0.SAMPCTRL = 31;
    }

    __inline__ bool ready() {
        // TODO should always return false if in the audio recording mode. How to determine? 
        return (ADC0.INTFLAGS & ADC_RESRDY_bm);
    }

    void processResult() {
        uint16_t value = ADC0.RES / 64;
        switch (ADC0.MUXPOS) {
            /* Calculates the VCC of the chip in 10mV increments, i.e. [V * 100]. Expected values are 0..500.
             */
            case MUXPOS_VCC:
                value = 110 * 512 / value;
                value = value * 2;
                // TODO
                ADC0.MUXPOS = MUXPOS_VBATT;
                break;
            /* Using the VCC value, calculates the VBATT in 10mV increments, i.e. [V*100]. Expected values are 0..430.
             */
            case MUXPOS_VBATT:
                // TODO
                ADC0.MUXPOS = MUXPOS_PHOTORES;
                break;
            /* Calculates the photoresistor value. Expected range is 0..255 which should correspond to 0..3V3 voltage measured. 
             
                TODO should we switch here for the 4.3V reference? 
             */
            case MUXPOS_PHOTORES:
            // TODO
                ++accPhotores_;
                ADC0.MUXPOS = MUXPOS_MIC;
                break;
            /* Measures the sound, because we can really.

               TODO might be useful for sound level detecting perhaps. 
             */
            case MUXPOS_MIC:
                ADC0.MUXPOS = MUXPOS_TEMP;
                ADC0.CTRLC = ADC_PRESC_DIV16_gc | ADC_REFSEL_INTREF_gc | ADC_SAMPCAP_bm; // internal reference
                break;
            /* Calculates the temperature in [C * 10] with 0.5 degree precission. 
             */
            case MUXPOS_TEMP: {
                // taken from the ATTiny datasheet example
                int8_t sigrow_offset = SIGROW.TEMPSENSE1; 
                uint8_t sigrow_gain = SIGROW.TEMPSENSE0;
                int32_t temp = value - sigrow_offset; // Result might overflow 16 bit variable (10bit+8bit)
                temp *= sigrow_gain;
                // temp is now in kelvin range, to convert to celsius, remove -273.15 (x256)
                temp -= 69926;
                // and now loose precision to 0.5C (x10, i.e. -15 = -1.5C)
                temp = (temp >>= 7) * 5;
                // fallthrough
            }
            default:
                ADC0.MUXPOS = MUXPOS_VCC;
                ADC0.CTRLC = ADC_PRESC_DIV16_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm; // use VDD as reference for VCC sensing
                break;
        }
        // start new conversion
        ADC0.COMMAND = ADC_STCONV_bm;
    }

}

/** \name ADC1
 
    ADC1 is used exclusively for reading the thumbstick state. It cycles through reading the horizontal, button and vertical value. The values are read all the time and are averaged on each tick. 
 */
namespace adc1 {

    constexpr uint8_t MUXPOS_JOY_H = ADC_MUXPOS_AIN6_gc;
    constexpr uint8_t MUXPOS_JOY_BTN = ADC_MUXPOS_AIN7_gc;
    constexpr uint8_t MUXPOS_JOY_V = ADC_MUXPOS_AIN8_gc;

    uint16_t accH_;
    uint16_t accV_;
    uint8_t accHSize_;
    uint8_t accVSize_;

    void initialize() {
        static_assert(JOY_H == 10); // AIN6 - ADC1, PC0
        PORTC.PIN0CTRL &= ~PORT_ISC_gm;
        PORTC.PIN0CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTC.PIN0CTRL &= ~PORT_PULLUPEN_bm;
        static_assert(JOY_BTN == 11); // AIN7 - ADC1, PC1
        PORTC.PIN1CTRL &= ~PORT_ISC_gm;
        PORTC.PIN1CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTC.PIN1CTRL &= ~PORT_PULLUPEN_bm;
        static_assert(JOY_V == 12); // AIN8 - ADC1, PC2
        PORTC.PIN2CTRL &= ~PORT_ISC_gm;
        PORTC.PIN2CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTC.PIN2CTRL &= ~PORT_PULLUPEN_bm;
    }

    void start() {
        accH_ = 0;
        accV_ = 0;
        accHSize_ = 0;
        accVSize_ = 0;
        // delay & sample averaging for increased precision
        ADC1.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_8BIT_gc;
        ADC1.CTRLB = ADC_SAMPNUM_ACC64_gc;
        ADC1.MUXPOS = MUXPOS_JOY_H;
        ADC1.CTRLC = ADC_PRESC_DIV16_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm; // use VDD as reference 
        ADC1.CTRLD = ADC_INITDLY_DLY32_gc;
        ADC1.SAMPCTRL = 31;
    }

    __inline__ bool ready() {
        return (ADC1.INTFLAGS & ADC_RESRDY_bm);
    }

    void processResult() {
        uint8_t value = ADC1.RES / 64;
        switch (ADC1.MUXPOS) {
            case MUXPOS_JOY_H:
                accH_ += value;
                ++accHSize_;
                ADC1.MUXPOS = MUXPOS_JOY_BTN;
                break;
            case MUXPOS_JOY_BTN:
                // TODO how to aggregate the button
                ADC1.MUXPOS = MUXPOS_JOY_V;
                break;
            case MUXPOS_JOY_V:
                accV_ += value;
                ++accVSize_;
            default:
                ADC1.MUXPOS = MUXPOS_JOY_H;
                break;
        }
        // start new conversion
        ADC1.COMMAND = ADC_STCONV_bm;
    }
}

/** \name Audio
 
    The audio module supports recording sound from the microphone in 8kHz 8bit resolution. When recording, the ADC0 is taken over by the audio subsystem and therefore no other measurements are made. This should not be a problem as recordings should be relatively shortlived, a few seconds max so that we can't run out of battery catastrophically while recording. 

    TODO Determine what gives the cleanest results. The more we oversample the better likely. 
 */
namespace audio {

    void startRecording() {

    }

    void stopRecording() {

    }

}

ISR(ADC0_RESRDY_vect) {
    /*
    Player::recordingBuffer_[Player::recordingWrite_++] = (ADC1.RES / 8) & 0xff;
    if (Player::recordingWrite_ % 32 == 0 && Player::status_.recording)
        Player::setIrq();
    */
}


/** \name Neopixel control. 
 */
namespace led {
    void off() {
        gpio::input(RGB_EN);
    }

    void on() {
        gpio::low(RGB_EN);
    }

}

/** \name AVR Modes

    Standby

    Brings the chip to the standby mode where we need to determine whether to go back to sleep, enter the torch mode or turn the rpi on. Also starts the battery voltage measurements so that if battery is depleted we won't enter any and go to sleep immediately, flashing the rgb red if things look bleak.

    Sleep

    Puts the chip to sleep, disabling all peripherals. 

    RpiPowerOn

    This function is called as soon as simultaneous press of L and R volume buttons is detected. It powers on the rpi without turning the backlight or vibration motor on. This allows the time required for the long press of L and R buttons to be part of the power on sequence, which subjectively shortens the boot sequence at the expense of extra battery drain of the buttons won't be pressed for long enough. 

    It is only after the long press and call to rpiOn() that the rpi power on will be made permanent and the display backlight will be turned on. 

    RpiOn

    Vibrates to signal rpi has been powered on and turns the display backlight on. Initializes the other systems such as audio & user inputs as at this time it is certain the rpi will power on (modulo low battery warning). 

    TorchOn

    RPI is off. the RGB led is on and the device can be used as a flashlight. 
 
    TODO timeout in seconds, TODO flashlight mode controls. 
 */
namespace mode {
    void standby() {
        adc0::initialize();
        adc0::start();
    }

    /** Puts the chip to sleep, disabling all peripherals. 
     */
    void sleep() {
        // disable rpi, disable rgb
        rpi::off();
        led::off();
        // disable backlight
        TCB0.CTRLA = 0;
        gpio::input(BACKLIGHT);
        // TODO vibration on 
        // TODO disable all timers but RTC
        // TODO disable vibration
        // TODO sleep
    }

    /** Turns on the power to rpi without any initialization
     */
    void rpiPowerOn() {
        // turn rpi on
        rpi::on();
    }

    /** Initializes the rpi and notifies user it is booting. 
     */
    void rpiOn() {
        // TODO vibrate

        // display backlight full on
        gpio::output(BACKLIGHT);
        TCB0.CTRLB = TCB_CNTMODE_PWM8_gc | TCB_CCMPEN_bm;
        TCB0.CCMPL = 255;
        TCB0.CCMPH = 255;
        TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm;
        // initialize ADC1
        adc1::initialize();
        adc1::start();

        // TODO set status to rpi mode
    }

    /** Enters the torch mode. 
     */
    void torchOn() {
        // turn the RGB power on 
        gpio::output(RGB_EN);
        gpio::low(RGB_EN);
        // RGB light at full, white color
        // TODO
        // TODO set status to torch mode
    }
} // namespace mode


/** The setup routine. 
 
    Called when the chip powers on, initializes the perihperals and gpios. Sets the power on flag and makes sure the rpi is on. 
 */
void setup() {
    // initialize common digital pins
    // make sure rpi is on, and clear the AVR_IRQ flag (will be pulled high by rpi)
    gpio::input(RPI_EN);
    gpio::input(AVR_IRQ);
    // start pullups on the L and R volume buttons
    gpio::inputPullup(BTN_LVOL);
    gpio::inputPullup(BTN_RVOL);
    // start reading the charging status
    gpio::input(CHARGE);
    // disable the rgb, brightness and vibration motor controls
    gpio::input(RGB_EN);
    gpio::input(BACKLIGHT);
    gpio::input(VIB_EN);
    // set the power on flag in config
    communication.state.setPowerOn();
    clocks::startRTC();
    // enter standby mode, then start rpi and make the start permanent immediately
    mode::standby();
    mode::rpiPowerOn();
    mode::rpiOn();
}

/** Main loop. 
 */
void loop() {
    if (flags.i2cReady)
        rpi::processCommand();
    if (adc0::ready())
        adc0::processResult();
    if (adc1::ready())
        adc1::processResult();
    /*
    if (ADC0.INTFLAGS & ADC_RESRDY_bm)
        processADC0Result();
    if (ADC1.INTFLAGS & ADC_RESRDY_bm)
        processADC1Result();
    if (flags.tick)
        tick();
    if (flags.sleep)
        sleep();
    // TODO this should be done only when rp calls us on i2c
    wdt::reset();
    */

}

// OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD

#ifdef FOO

void setup2() {

    gpio::output(DEBUG_PIN);

    // set IRQ pin as input (is pulled up by RP2040 when active)
    gpio::input(IRQ_PIN);
    // button pins set to input 
    gpio::inputPullup(BTN_START_PIN);
    gpio::inputPullup(BTN_SELECT_PIN);
    // joy pins set to analog inputs
    static_assert(JOY_X_PIN == 12); // AIN8, PC2
    PORTC.PIN2CTRL &= ~PORT_ISC_gm;
    PORTC.PIN2CTRL |= PORT_ISC_INPUT_DISABLE_gc;
    PORTC.PIN2CTRL &= ~PORT_PULLUPEN_bm;
    static_assert(JOY_Y_PIN == 13); // AIN9, PC3
    PORTC.PIN3CTRL &= ~PORT_ISC_gm;
    PORTC.PIN3CTRL |= PORT_ISC_INPUT_DISABLE_gc;
    PORTC.PIN3CTRL &= ~PORT_PULLUPEN_bm;
    // initialize the RTC to fire every second
    RTC.CLKSEL = RTC_CLKSEL_INT1K_gc; // select internal oscillator divided by 32
    RTC.PITINTCTRL |= RTC_PI_bm; // enable the interrupt
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
    ADC1.MUXPOS = ADC_MUXPOS_AIN8_gc; // PC2, JOY_X
    ADC1.CTRLC = ADC_PRESC_DIV16_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm; // use VDD as reference 
    ADC1.CTRLD = ADC_INITDLY_DLY32_gc;
    ADC1.SAMPCTRL = 31;
    // attach the power button to interrupt so that it can wake us up
    attachInterrupt(digitalPinToInterrupt(BTN_SELECT_PIN), pwrButtonDown, CHANGE);
    // enable the display brightness PWM timer, fully on
    gpio::output(DISP_BRIGHTNESS_PIN);
    TCB1.CTRLB = TCB_CNTMODE_PWM8_gc | TCB_CCMPEN_bm;
    TCB1.CCMPL = 255;
    TCB1.CCMPH = 255;
    TCB1.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm;
    // start the 1kHz timer for ticks
    // TODO change this to 8kHz for audio recording, or use different timer? 
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;
    TCB0.INTCTRL = TCB_CAPT_bm;
    TCB0.CCMP = 10000; // for 1kHz    
    TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm;
    // wakeup, including power to pico
    wakeup();
}

/** Determines if there is a change in state for given button. 
 */
bool checkButtons() {
    bool changed = false;
    bool value = gpio::read(BTN_START_PIN);
    if (buttonTimers[0] == 0 && communication.state.btnStart() != value) {
        communication.state.setBtnStart(value);
        cli();
        buttonTimers[0] = BUTTON_DEBOUNCE;
        sei();
        changed = true;
    } 
    value = gpio::read(BTN_SELECT_PIN);
    if (buttonTimers[1] == 0 && communication.state.btnSelect() != value) {
        communication.state.setBtnSelect(value);
        cli();
        buttonTimers[1] = BUTTON_DEBOUNCE;
        sei();
        changed = true;
    } 
    return changed;
}


void tick() {
    flags.tick = false;
    bool irq = flags.irq;
    // check buttons 
    for (int i = 0; i < NUM_BUTTONS; ++i)
        irq = checkButtons() || irq;
    // set irq if it is requested by either the 
    if (irq && gpio::read(IRQ_PIN)) {
        flags.irq = false;
        gpio::output(IRQ_PIN);
        gpio::low(IRQ_PIN);
    }
}

void wakeup() {
    // clear the irq
    gpio::input(IRQ_PIN);




    // start conversions on the ADCs
    ADC0.COMMAND = ADC_STCONV_bm;
    // TODO check the voltage level before starting pico




    // power up the thumbstick and start its sampling
    gpio::output(JOY_PWR_PIN);
    gpio::high(JOY_PWR_PIN);
    ADC1.COMMAND = ADC_STCONV_bm;
    // start RP2040, initialize I2C
    i2c::initializeSlave(comms::AVR_I2C_ADDRESS);
    gpio::input(EN_3V3_PIN);
}

void sleep() {
    // turn off RP2040
    gpio::output(EN_3V3_PIN);
    gpio::low(EN_3V3_PIN);
    // turn off joystick power
    gpio::input(JOY_PWR_PIN);
    while (true) {
        // disable the tick timer
        TCB0.CTRLA &= ~ TCB_ENABLE_bm;
        // clear the button debounce timeouts
        for (int i = 0; i < NUM_BUTTONS; ++i)
            buttonTimers[i] = 0;
        // while sleeping, disable watchdog, make sure that any interrupts that would wake up will put to sleep immediately
        wdt::disable();
        while (flags.sleep)
            cpu::sleep();
        wdt::enable();
        TCB0.CTRLA |= TCB_ENABLE_bm;
        // avr wakes up immediately after PWR button is held down. Wait some time for the pwr button to be pressed down 
        uint16_t ticks = POWERON_PRESS;
        tick();
        while (communication.state.btnStart()) {
            if (flags.tick) {
                tick();
                if (--ticks == 0) {
                    wakeup();
                    return;
                }
            }
        }
        // premature release, go back to sleep
        flags.sleep = true;
    }
}

void loop() {
    if (flags.i2cReady)
        processCommand();
    if (ADC0.INTFLAGS & ADC_RESRDY_bm)
        processADC0Result();
    if (ADC1.INTFLAGS & ADC_RESRDY_bm)
        processADC1Result();
    if (flags.tick)
        tick();
    if (flags.sleep)
        sleep();
    // TODO this should be done only when rp calls us on i2c
    wdt::reset();
}

/** IRQ for power button change. If sleeping, wake up from sleep. The sleep function will take care of the rest. 
 */
void pwrButtonDown() {
    if (flags.sleep)
        flags.sleep = false;
}

/** Switch debouncing 1kHz interrupt. 
 */
ISR(TCB0_INT_vect) {
    //gpio::high(DEBUG_PIN);    
    TCB0.INTFLAGS = TCB_CAPT_bm;
    for (int i = 0; i < NUM_BUTTONS; ++i)
        if (buttonTimers[i] > 0)
            --buttonTimers[i];
    // main loop tick for checking button values
    flags.tick = true;
    //gpio::low(DEBUG_PIN);
}

#endif