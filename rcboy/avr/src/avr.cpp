#include <Arduino.h>

#include "platform/platform.h"

#include "comms.h"

#include "peripherals/neopixel.h"

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


/** The entire status of the AVR as a continuous area of memory so that when we rpi reads the I2C all this information can be returned. 
 */
struct {
    comms::Status status;
    uint8_t i2cBuffer[comms::I2C_BUFFER_SIZE];
    uint8_t i2cBytesRecvd = 0;
    uint8_t i2cBytesSent = 0;
    volatile struct {
        /** Determines that the AVR irq is either requested, or should be requested at the next tick. 
         */
        bool irq : 1;
        bool i2cReady : 1;
        bool tick : 1; 
        bool secondTick : 1;
        bool sleep : 1;
    } flags;
} state;

/** \name Clocks
 
    Simply increases the time by one second and sets the second tick flag so that the main loop can react to the second increase in due time. 
 */
namespace clocks {

    /** Normally the arduino core uses TCA0 timer to generate PWM signals on the analog pins. Since we do not use these, we can take over TCA0. Or we can use analog write to backlight and vibration motor via TCA and use TCB for the ticks & audio timing. 
     */
    void initialize() {
        /*
        takeOverTCA0(); 
        TCA0.SINGLE.CTRLD = 0; // make sure we are not iin split mode
        */
    }

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
        TCB0.CTRLB = TCB_CNTMODE_INT_gc;
        TCB0.CCMP = 40000; // for 100Hz

        /*
        TCA0.SINGLE.PER = 52083;
        TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm;
        */
        TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
    }

    bool tick() {
        if (TCB0.INTFLAGS & TCB_CAPT_bm) {
            TCB0.INTFLAGS = TCB_CAPT_bm;
            return true;
        } else {
            return false;
        }
        /*
        if (TCA0.SPLIT.INTFLAGS & TCA_SPLIT_HUNF_bm) {
            TCA0.SPLIT.INTFLAGS |= TCA_SPLIT_HUNF_bm;
            return true;
        } else {
            return false;
        } */
        /*
        
        if (TCA0.SINGLE.INTFLAGS & TCA_SINGLE_OVF_bm) {
            TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_OVF_bm;
            return true;
        } else {
            return false;
        }
        */
       return true;
        
    }

    void stopTick() {
        // if there was a pending tick, disable it
        state.flags.tick = false;

    }
}

ISR(RTC_PIT_vect) {
    RTC.PITINTFLAGS = RTC_PI_bm; // clear the interrupt
    state.status.time().secondTick();
    state.flags.secondTick = true;
}

/** \name Interface with the RPi.
 
    Communication is done via I2C and the IRQ pin that is pulled up by RPi and can be driven low by AVR if it wants to communicate a state change to RPi. 
 */
namespace rpi {

    void initialize() {
        // make sure rpi is on, and clear the AVR_IRQ flag (will be pulled high by rpi)
        gpio::input(RPI_EN);
        gpio::input(AVR_IRQ);
    }

    void on() {
        gpio::input(RPI_EN);
        // start the backlight timer
        
    }

    void off() {
        gpio::output(RPI_EN);
        gpio::high(RPI_EN);
        // disable the backlight timer

    }

    void setIrq() {
        state.flags.irq = true;
    }

    void setIrqImmediately() {
        state.flags.irq = true;
        gpio::output(AVR_IRQ);
        gpio::low(AVR_IRQ);
    }

    void tick() {
        // force IRQ down if it's up and the irq flag is set
        if (state.flags.irq == true && gpio::read(AVR_IRQ)) {
            gpio::output(AVR_IRQ);
            gpio::low(AVR_IRQ);
        }
    }

    void processCommand() {
        // TODO

        // clear the received bytes buffer and the i2c command ready flag
        state.i2cBytesRecvd = 0;
        cli();
        state.flags.i2cReady = false;
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
        if (state.i2cBytesSent < sizeof (state.status) + sizeof (state.i2cBuffer)) {
            TWI0.SDATA = reinterpret_cast<uint8_t*>(& state.status)[state.i2cBytesSent++];
            TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
        } else {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
        }
    // a byte has been received from master. Store it and send either ACK if we can store more, or NACK if we can't store more
    } else if ((status & I2C_DATA_MASK) == I2C_DATA_RX) {
        state.i2cBuffer[state.i2cBytesRecvd++] = TWI0.SDATA;
        TWI0.SCTRLB = (state.i2cBytesRecvd == sizeof(state.i2cBuffer)) ? TWI_SCMD_COMPTRANS_gc : TWI_SCMD_RESPONSE_gc;
    // master requests slave to write data, clear the IRQ and prepare to send the state
    } else if ((status & I2C_START_MASK) == I2C_START_TX) {
        gpio::input(AVR_IRQ);
        state.i2cBytesSent = 0;
        TWI0.SCTRLB = TWI_ACKACT_ACK_gc + TWI_SCMD_RESPONSE_gc;
        //gpio::input(IRQ_PIN); // clear IRQ
    // master requests to write data itself. ACK if there is no pending I2C message, NACK otherwise
    } else if ((status & I2C_START_MASK) == I2C_START_RX) {
        TWI0.SCTRLB = state.flags.i2cReady ? TWI_ACKACT_NACK_gc : TWI_SCMD_RESPONSE_gc;
    // sending finished
    } else if ((status & I2C_STOP_MASK) == I2C_STOP_TX) {
        TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
    // receiving finished, inform main loop we have message waiting
    } else if ((status & I2C_STOP_MASK) == I2C_STOP_RX) {
        TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
        state.flags.i2cReady = true;
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

    void tick() {
        // TODO update the photoresistor value
    }

    bool ready() {
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

    NeopixelStrip<1> led_{RGB};
    ColorStrip<1> desired_;    

    void initialize() {
        gpio::input(RGB_EN);
    }

    void off() {
        gpio::input(RGB_EN);
    }

    void on() {
        gpio::output(RGB_EN);
        gpio::low(RGB_EN);
    }

    void setColor(Color c) {
        desired_[0] = c;
    }

    void tick() {
        if (led_.moveTowards(desired_))
            led_.update();
    }

}


/** \name User Inputs

    User input is three digital buttons (L,R and JOY) and the analog joystick horizontal and vertical axis. The buttons work at all times (using internal pullups from the AVR), but the analog stick works only when RPI is on as it is powered from the audio 3V3 power rail. 

    The left and right volume buttons are protected against accidental presses and therefore are used to turn the device on or enable the torch mode via interrupts attached to them. 

    All three buttons are debounced.

 */
namespace inputs {

    constexpr uint8_t DEBOUNCE_TICKS = 10;

    constexpr uint8_t MUXPOS_JOY_H = ADC_MUXPOS_AIN6_gc;
    constexpr uint8_t MUXPOS_JOY_V = ADC_MUXPOS_AIN8_gc;

    // joystick H and V values are aggregated and only reported at each tick
    uint16_t accH_;
    uint16_t accV_;
    uint8_t accHSize_;
    uint8_t accVSize_;

    uint8_t debounceCounter_[3] = {0,0,0};

    // forward declaration for the ISR
    void leftVolume();
    void rightVolume();
    void joystickButton();

    void initialize() {
        // start pullups on the L and R volume buttons
        gpio::inputPullup(BTN_LVOL);
        gpio::inputPullup(BTN_RVOL);
        gpio::input(JOY_BTN);
        // attach button interrupts on change
        attachInterrupt(digitalPinToInterrupt(BTN_LVOL), leftVolume, CHANGE);
        attachInterrupt(digitalPinToInterrupt(BTN_RVOL), rightVolume, CHANGE);
        // initialize ADC1 responsible for the joystick position
        static_assert(JOY_H == 10); // AIN6 - ADC1, PC0
        PORTC.PIN0CTRL &= ~PORT_ISC_gm;
        PORTC.PIN0CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTC.PIN0CTRL &= ~PORT_PULLUPEN_bm;
        static_assert(JOY_V == 12); // AIN8 - ADC1, PC2
        PORTC.PIN2CTRL &= ~PORT_ISC_gm;
        PORTC.PIN2CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTC.PIN2CTRL &= ~PORT_PULLUPEN_bm;
    }

    void joystickStart() {
        gpio::inputPullup(JOY_BTN);

        accH_ = 0;
        accV_ = 0;
        accHSize_ = 0;
        accVSize_ = 0;
        // delay & sample averaging for increased precision
        ADC1.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc;
        ADC1.CTRLB = ADC_SAMPNUM_ACC64_gc;
        ADC1.MUXPOS = MUXPOS_JOY_H;
        ADC1.CTRLC = ADC_PRESC_DIV16_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm; // use VDD as reference 
        ADC1.CTRLD = ADC_INITDLY_DLY32_gc;
        ADC1.SAMPCTRL = 31;
        // TODO switch reference to 4V3 for better resolution? Most likely not as it might not be available when powering from less than that 
        attachInterrupt(digitalPinToInterrupt(JOY_BTN), joystickButton, CHANGE);
    }

    /** Disables ADC1 and interrupt on the joystick button so that AVR won't get interrupts from it. 
     */
    void joystickStop() {
        ADC1.CTRLA = 0;
        detachInterrupt(digitalPinToInterrupt(JOY_BTN));
        gpio::input(JOY_BTN);
    }

    bool joystickReady() {
        return (ADC1.INTFLAGS & ADC_RESRDY_bm);
    }

    void processJoystick() {
        uint8_t value = ADC1.RES / 64;
        switch (ADC1.MUXPOS) {
            case MUXPOS_JOY_H:
                accH_ += value;
                ++accHSize_;
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

    void leftVolume() {
        if (debounceCounter_[0] == 0) {
            debounceCounter_[0] = DEBOUNCE_TICKS;
            state.status.setBtnLeftVolume(gpio::read(BTN_LVOL));
            rpi::setIrq();
        }
    }

    void rightVolume() {
        if (debounceCounter_[1] == 0) {
            debounceCounter_[1] = DEBOUNCE_TICKS;
            state.status.setBtnRightVolume(gpio::read(BTN_RVOL));
            rpi::setIrq();
        }
    }

    void joystickButton() {
        if (debounceCounter_[2] == 0) {
            debounceCounter_[2] = DEBOUNCE_TICKS;
            state.status.setBtnJoystick(gpio::read(JOY_BTN));
            rpi::setIrq();
        }
    }

    /** Each tick decrements the debounce timers and if they reach zero, the button status if updated and if it changes, irq is set (in this case press or release was shorted than the debounce interval). During the tick we also update the X and Y joystick readings (averaged over the measurements that happened during the tick for less jitter). 
     */
    void tick() {
        if (debounceCounter_[0] > 0)
            if (--debounceCounter_[0] == 0)
                if (state.status.setBtnLeftVolume(gpio::read(BTN_LVOL)))
                    rpi::setIrq();
        if (debounceCounter_[0] > 0)
            if (--debounceCounter_[0] == 0)
                if (state.status.setBtnRightVolume(gpio::read(BTN_RVOL)))
                    rpi::setIrq();
        if (debounceCounter_[0] > 0)
            if (--debounceCounter_[0] == 0)
                if (state.status.setBtnJoystick(gpio::read(JOY_BTN)))
                    rpi::setIrq();
        if (accHSize_ != 0) {
            accH_ = accH_ / accHSize_;
            if (state.status.setJoyX(accH_))
                rpi::setIrq();
        }
        if (accVSize_ != 0) {
            accV_ = accV_ / accVSize_;
            if (state.status.setJoyY(accV_))
                rpi::setIrq();
        }
        accH_ = 0;
        accV_ = 0;
        accHSize_ = 0;
        accVSize_ = 0;
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
        wdt::enable();
        adc0::initialize();
        adc0::start();
        clocks::startTick();
    }

    /** Puts the chip to sleep, disabling all peripherals. 
     */
    void sleep() {
        // disable the ticks counter
        clocks::stopTick();
        // disable rpi, disable rgb
        rpi::off();
        led::off();
        inputs::joystickStop();
        // disable backlight
        TCB0.CTRLA = 0;
        gpio::input(BACKLIGHT);
        // TODO vibration on 
        // TODO disable all timers but RTC
        // TODO disable vibration
        // TODO sleep
        wdt::disable();
        while (state.flags.sleep)
            cpu::sleep();
        standby();
        // wait for the mode to be set either to rpi, torch, or back to sleep
    }

    /** Turns on the power to rpi without any initialization
     */
    void rpiPowerOn() {
        // turn rpi on
        rpi::on();
        // initalize I2C slave if the pi wants to talk to us
        i2c::initializeSlave(comms::AVR_I2C_ADDRESS);

    }

    /** Initializes the rpi and notifies user it is booting. 
     */
    void rpiOn() {
        // TODO vibrate

        // display backlight full on
        /*
        gpio::output(BACKLIGHT);
        TCB0.CTRLB = TCB_CNTMODE_PWM8_gc | TCB_CCMPEN_bm;
        TCB0.CCMPL = 255;
        TCB0.CCMPH = 255;
        TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm;
        */
        // start joystick readouts
        inputs::joystickStart();

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
    clocks::initialize();
    // initialize common digital pins
    rpi::initialize();
    inputs::initialize();
    led::initialize();
    // start reading the charging status
    gpio::input(CHARGE);
    // disable the rgb, brightness and vibration motor controls
    gpio::input(BACKLIGHT);
    gpio::input(VIB_EN);
    // set the power on flag in config
    state.status.setPowerOn();
    clocks::startRTC();
    // enter standby mode, then start rpi and make the start permanent immediately
    mode::standby();
    mode::rpiPowerOn();
    mode::rpiOn();

    led::on();
    for (int i = 0; i < 5; ++i) {
        led::setColor(Color::White().withBrightness(32));
        led::tick();
        cpu::delay_ms(200);
        led::setColor(Color::Black());
        led::tick();
        cpu::delay_ms(200);
    }
    //led::off();

    //gpio::input(BACKLIGHT);
    gpio::output(BACKLIGHT);
    analogWrite(BACKLIGHT, 128);
    gpio::output(VIB_EN);
    //analogWrite(VIB_EN, 128);
    CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA |= CLKCTRL_CLKOUT_bm;
    CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLB = 0;

}

bool x;

/** Main loop. 
 */
void loop() {
    if (state.flags.i2cReady)
        rpi::processCommand();
    if (adc0::ready())
        adc0::processResult();
    if (inputs::joystickReady())
        inputs::processJoystick();



    if (clocks::tick()) {
        if (x)
            led::setColor(Color::White().withBrightness(32));
        else
            led::setColor(Color::Black());
        x = !x;
        digitalWrite(VIB_EN, x);
        adc0::tick();
        led::tick();
        inputs::tick();
        // the rpi tick must be last as it may set the irq if any of the ticks above determine it need to be set
        rpi::tick();
        // TODO should we do this only when rp calls us on IRQ? likely not 
        wdt::reset();
         
    }
    // if there is 
    //if (state.flags.sleep)
    //    mode::sleep();
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