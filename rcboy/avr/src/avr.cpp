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
    /** State
     */
    comms::Status status;
    comms::ExtendedStatus estatus;
    DateTime time;

    /** Audio buffer
     */
    uint8_t buffer[comms::I2C_PACKET_SIZE * 2];

} state;

volatile struct {
    /** Determines that the AVR irq is either requested, or should be requested at the next tick. 
     */
    bool irq : 1;
    bool i2cReady : 1;
    bool secondTick : 1;
    bool sleep : 1;
} flags;


/** \name Clocks
 
    Simply increases the time by one second and sets the second tick flag so that the main loop can react to the second increase in due time. 
 */
namespace clocks {

    /** Initializes the clocks. 
     */
    void initialize() {
        // for debugging purposes, see the CLK_MAIN on PB5
        //CCP = CCP_IOREG_gc;
        //CLKCTRL.MCLKCTRLA |= CLKCTRL_CLKOUT_bm;
        // disable CLK_PER prescaler, i.e. CLK_PER = CLK_MAIN
        CCP = CCP_IOREG_gc;
        CLKCTRL.MCLKCTRLB = CLKCTRL_PEN_bm;
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
     
         The tick counter uses the TCB0 timer with frequency of 100Hz, i.e. a tick every 10ms, which seems to be acceptable lag without ticking too often.
     */
    void startTick() {
        TCB0.CTRLB = TCB_CNTMODE_INT_gc;
        TCB0.CCMP = 40000; // for 100Hz
        TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
    }

    bool tick() {
        if (TCB0.INTFLAGS & TCB_CAPT_bm) {
            TCB0.INTFLAGS = TCB_CAPT_bm;
            return true;
        } else {
            return false;
        }
    }

    void stopTick() {
        TCB0.CTRLA = 0;
        // if there was a pending tick, disable it
        TCB0.INTFLAGS = TCB_CAPT_bm;
    }
}

ISR(RTC_PIT_vect) {
    RTC.PITINTFLAGS = RTC_PI_bm; // clear the interrupt
    state.time.secondTick();
    flags.secondTick = true;
}

/** \name Interface with the RPi.
 
    Communication is done via I2C, where the RPi is the master and AVR is the slave. When the AVR wants to signal RPi that a state has been changed, it pulls the AVR_IRQ pin low. 

    The RPi can either read data from the AVR, or send commands. The commands can consist of up to 32 bytes and are received in a dedicated buffer. When the master write transaction ends the avr signals the main thread to process the command received. Only one command can be received at a time. Attempting to send a command while the previous command is being processed results in a NACK. 
    
 */
namespace rpi {

    /** The desired address from which a new read should start. Note that each read is always 32 bytes long. 
     */
    uint8_t * i2cReadStart_ = reinterpret_cast<uint8_t*>(& state);
    uint8_t * i2cReadActual_;
    uint8_t i2cBytesRecvd_ = 0;
    uint8_t i2cBytesSent_ = 0;

    /** I2C buffer.
     */
    uint8_t i2cBuffer_[comms::I2C_BUFFER_SIZE];


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

    }

    void setIrq() {
        flags.irq = true;
    }

    void setIrqImmediately() {
        flags.irq = true;
        gpio::output(AVR_IRQ);
        gpio::low(AVR_IRQ);
    }

    void tick() {
        // force IRQ down if it's up and the irq flag is set
        if (flags.irq == true && gpio::read(AVR_IRQ)) {
            gpio::output(AVR_IRQ);
            gpio::low(AVR_IRQ);
        }
    }

    void processCommand() {
        msg::Message const & m = msg::Message::fromBuffer(i2cBuffer_);
        switch (i2cBuffer_[0]) {
            case msg::ClearPowerOnFlag::Id: {
                state.status.setAvrPowerOn(false);
                break;
            }
            case msg::SetBrightness::Id: {
                state.estatus.setBrightness(m.as<msg::SetBrightness>().value);
                analogWrite(BACKLIGHT, state.estatus.brightness());
                break;
            }

        }

        // clear the received bytes buffer and the i2c command ready flag
        i2cBytesRecvd_ = 0;
        cli();
        flags.i2cReady = false;
        sei();
    }

    /** Sets the address from which the next I2C read will be. 
     */
    void setNextReadAddress(uint8_t * addr) {
        cli();
        i2cReadStart_ = addr;
        sei();
    }

    /** I2C Slave handler. 
     
        For master writes, all data is stored in the i2cBuffer. When the transaction completes, the i2cReady flag is set and the main app can process the received command. 

        TODO can read either the state (always), or can read audio, in which case we remember where to start reading. After 32 bits we always switch to state. No other way to specify where to read. 

        OK? 

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
        uint8_t status = TWI0.SSTATUS;
        // sending data to accepting master is on our fastpath as is checked first, if there is more state to send, send next byte, otherwise go to transcaction completed mode. 
        if ((status & I2C_DATA_MASK) == I2C_DATA_TX) {
            if (i2cBytesSent_ >= comms::I2C_PACKET_SIZE) {
                i2cReadActual_ = reinterpret_cast<uint8_t*>(& state);
                i2cBytesSent_ = 0;
            }
            TWI0.SDATA = i2cReadActual_[i2cBytesSent_++];
            TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
            //TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
        // a byte has been received from master. Store it and send either ACK if we can store more, or NACK if we can't store more
        } else if ((status & I2C_DATA_MASK) == I2C_DATA_RX) {
            i2cBuffer_[i2cBytesRecvd_++] = TWI0.SDATA;
            TWI0.SCTRLB = (i2cBytesRecvd_ == sizeof(i2cBuffer_)) ? TWI_SCMD_COMPTRANS_gc : TWI_SCMD_RESPONSE_gc;
        // master requests slave to write data, reset the sent bytes counter, initialize the actual read address from the read start and reset the IRQ
        } else if ((status & I2C_START_MASK) == I2C_START_TX) {
            gpio::input(AVR_IRQ);
            i2cBytesSent_ = 0;
            i2cReadActual_ = i2cReadStart_;
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

    uint8_t micMax_;

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
        micMax_ = 0;
        // voltage reference to 1.1V (internal for the temperature sensor)
        VREF.CTRLA &= ~ VREF_ADC0REFSEL_gm;
        VREF.CTRLA |= VREF_ADC0REFSEL_1V1_gc;
        // delay 32us and sampctrl of 32 us for the temperature sensor, do averaging over 64 values, full precission
        ADC0.CTRLB = ADC_SAMPNUM_ACC64_gc;
        ADC0.MUXPOS = MUXPOS_VCC; // start by measuring the internal voltage 
        ADC0.CTRLC = ADC_PRESC_DIV16_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm; // use VDD as reference for VCC sensing, 0.5Mhz
        ADC0.CTRLD = ADC_INITDLY_DLY32_gc;
        ADC0.SAMPCTRL = 31;
        ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc;
        ADC0.COMMAND = ADC_STCONV_bm;
     }

    void tick() {
        // check the microphone threshold 
        if (state.estatus.irqMic() && micMax_ >= state.estatus.micThreshold()) 
            if (state.status.setMicLoud())
                rpi::setIrq();
        micMax_ = 0;
    }

    bool ready() {
        // the ADC is never ready when recording 
        if (ADC0.CTRLA & ADC_FREERUN_bm)
            return false;
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
            /* Calculates the photoresistor value. Expected range is 0..255 which should correspond to 0..VCC voltage measured. 
             */
            case MUXPOS_PHOTORES:
                if (state.status.setPhotores((value >> 2) & 0xff) && state.estatus.irqPhotores())
                    rpi::setIrq();
                ADC0.MUXPOS = MUXPOS_MIC;
                break;
            /* Measures the sound, so that we can trigger the mic loud event if necessary. 
             */
            case MUXPOS_MIC:
                if (value > micMax_)
                    micMax_ = value;
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

    uint16_t recAcc_;
    uint8_t recAccSize_;
    uint8_t bufferIndex_;

    void startRecording() {
        // start the ADC first
        ADC0.CTRLA = 0; // disable ADC so that any pending read from the main app is cancelled
        // TODO set reference voltage to something useful, such as 2.5? 
        ADC0.MUXPOS = adc0::MUXPOS_MIC;
        ADC0.INTCTRL |= ADC_RESRDY_bm;
        ADC0.CTRLB = 0; // no sample accumulation
        ADC0.CTRLC = ADC_PRESC_DIV4_gc | ADC_REFSEL_INTREF_gc | ADC_SAMPCAP_bm;
        ADC0.CTRLD = 0; // no sample delay, no init delay
        ADC0.SAMPCTRL = 0;
        ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_8BIT_gc | ADC_FREERUN_bm;
        // then start the 8khz timer
        TCB1.CTRLB = TCB_CNTMODE_INT_gc;
        TCB1.CCMP = 500; // for 8kHz
        TCB1.INTCTRL = TCB_CAPT_bm;
        TCB1.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
        recAcc_ = 0;
        recAccSize_ = 0;
        bufferIndex_ = 0;
    }

    /** Stops the audio recording by disabling the 8kHz timer and returning ADC0 to the single conversion mode used to sample multiple perihperals and voltages. 
     */
    void stopRecording() {
        ADC0.CTRLA = 0;
        ADC0.INTCTRL = 0;
        TCB1.CTRLA = 0;
        adc0::start();
    }

    ISR(TCB1_INT_vect) {
        TCB1.INTFLAGS = TCB_CAPT_bm;
        if (recAccSize_ > 0) 
            recAcc_ /= recAccSize_;
        // store the value in buffer
        state.buffer[bufferIndex_] = (recAcc_ & 0xff);
        bufferIndex_ = (bufferIndex_ + 1) % (comms::I2C_PACKET_SIZE * 2);
        if (bufferIndex_ % comms::I2C_PACKET_SIZE == 0) {
            rpi::setNextReadAddress(bufferIndex_ == 0 ? state.buffer : state.buffer + comms::I2C_PACKET_SIZE);
            rpi::setIrqImmediately();
        }
        // reset the accumulator for next phase
        recAcc_ = 0;
        recAccSize_ = 0;
    }

    ISR(ADC0_RESRDY_vect) {
        recAcc_ += ADC0.RES;
        ++recAccSize_;    
    }

} // namespace audio

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

} // namespace led


/** \name User Inputs

    User input is three digital buttons (L,R and JOY) and the analog joystick horizontal and vertical axis. The buttons work at all times (using internal pullups from the AVR), but the analog stick works only when RPI is on as it is powered from the audio 3V3 power rail. 

    The left and right volume buttons are protected against accidental presses and therefore are used to turn the device on or enable the torch mode via interrupts attached to them. 

    All three buttons are debounced.

    NOTE that the vibration motor's PWM is reversed, i.e. PWM 254 = lowest setting, PWM 1 = higherst setting

 */
namespace inputs {

    constexpr uint8_t DEBOUNCE_TICKS = 2;

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
        // disable the vibration motor
        gpio::input(VIB_EN);
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
        ADC1.COMMAND = ADC_STCONV_bm;
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
            if (state.status.setBtnLeftVolume(gpio::read(BTN_LVOL)));
                rpi::setIrq();
            if (! gpio::read(BTN_LVOL))
                led::setColor(Color::Blue().withBrightness(32));
            else
                led::setColor(Color::Black());
        }
    }

    void rightVolume() {
        if (debounceCounter_[1] == 0) {
            debounceCounter_[1] = DEBOUNCE_TICKS;
            if (state.status.setBtnRightVolume(gpio::read(BTN_RVOL)));
                rpi::setIrq();
        }
    }

    void joystickButton() {
        if (debounceCounter_[2] == 0) {
            debounceCounter_[2] = DEBOUNCE_TICKS;
            if (state.status.setBtnJoystick(gpio::read(JOY_BTN)));
                rpi::setIrq();
            if (! gpio::read(JOY_BTN))
                led::setColor(Color::Green().withBrightness(32));
            else
                led::setColor(Color::Black());
        }
    }

    /** Each tick decrements the debounce timers and if they reach zero, the button status if updated and if it changes, irq is set (in this case press or release was shorted than the debounce interval). During the tick we also update the X and Y joystick readings (averaged over the measurements that happened during the tick for less jitter). 
     */
    void tick() {
        if (debounceCounter_[0] > 0)
            if (--debounceCounter_[0] == 0)
                if (state.status.setBtnLeftVolume(gpio::read(BTN_LVOL)))
                    rpi::setIrq();
        if (debounceCounter_[1] > 0)
            if (--debounceCounter_[1] == 0)
                if (state.status.setBtnRightVolume(gpio::read(BTN_RVOL)))
                    rpi::setIrq();
        if (debounceCounter_[2] > 0)
            if (--debounceCounter_[2] == 0)
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
        while (flags.sleep)
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
    state.status.setAvrPowerOn();
    clocks::startRTC();
    // enter standby mode, then start rpi and make the start permanent immediately
    mode::standby();
    mode::rpiPowerOn();
    mode::rpiOn();

    led::on();
    /*
    if (FUSE.OSCCFG == 1) {
        led::setColor(Color::Red());
        led::tick();
        cpu::delay_ms(200);
    }
    */
    for (int i = 0; i < 3; ++i) {
        led::setColor(Color::White().withBrightness(32));
        led::tick();
        cpu::delay_ms(100);
        led::setColor(Color::Black());
        led::tick();
        cpu::delay_ms(100);
    }
    //led::off();
    //gpio::input(BACKLIGHT);
    gpio::output(BACKLIGHT);
    analogWrite(BACKLIGHT, 128);
    /*
    CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA |= CLKCTRL_CLKOUT_bm;
    CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLB = 0;
    */

}

/** Main loop. 
 */
void loop() {
    if (flags.i2cReady)
        rpi::processCommand();
    if (adc0::ready())
        adc0::processResult();
    if (inputs::joystickReady())
        inputs::processJoystick();



    if (clocks::tick()) {
        adc0::tick();
        inputs::tick();
        // the rpi tick must be last as it may set the irq if any of the ticks above determine it need to be set
        rpi::tick();
        led::tick();
        // TODO should we do this only when rp calls us on IRQ? likely not 
        wdt::reset();
         
    }
    // if there is 
    if (flags.sleep)
        mode::sleep();
}