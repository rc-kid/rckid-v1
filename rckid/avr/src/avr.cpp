#include <Arduino.h>

#include "platform/platform.h"
#include "platform/peripherals/neopixel.h"

#include "common/comms.h"
#include "common/config.h"


using namespace platform;
using namespace comms;

#define ENTER_IRQ //gpio::high(RCKid::RGB)
#define LEAVE_IRQ //gpio::low(RCKid::RGB)

/** Static class containing all modules maintained by the AVR chip in RCKid.

    The AVR is connected diretly to the battery/usb-c power line and manages the power and many peripherals. To the RPI, AVR presents itself as an I2C slave and 2 dedicated pins are used to signal interrupts from AVR to RPI and to signal safe shutdown from RPI to AVR. The AVR is intended to run an I2C bootloader that can be used to update the AVR's firmware when necessary.

                   -- VDD             GND --
             VBATT -- (00) PA4   PA3 (16) -- VIB_EN
           MIC_OUT -- (01) PA5   PA2 (15) -- SCL (I2C)
            BTNS_2 -- (02) PA6   PA1 (14) -- SDA (I2C)
            BTNS_1 -- (03) PA7   PA0 (17) -- (reserved for UPDI)
             JOY_V -- (04) PB5   PC3 (13) -- BTN_HOME
             JOY_H -- (05) PB4   PC2 (12) -- RGB
            RGB_EN -- (06) PB3   PC1 (11) -- RPI_POWEROFF
            RPI_EN -- (07) PB2   PC0 (10) -- AVR_IRQ
            CHARGE -- (08) PB1   PB0 (09) -- BACKLIGHT

    # Design Blocks

    `RTC` is used to generate a 1 second interrupt for real time clock timekeeping, active even in sleep. 

    `ADC0` is used to capture the analog controls (JOY_H, JOY_V, BTNS_1, BTNS_2), power information (VBATT, CHARGE) and internally the VCC and temperature on AVR. ADC also generates a rough estimate of a tick when all measurements are cycled though. 

    `ADC1` is reserved for the microphone input at 8kHz. The ADC is left in a free running mode to accumulate as many results as possible, and `TCB0` is used to generate a precise 8kHz signal to average and capture the signal. 

    `TCA0` is used in split mode to generate PWM signals for the rumbler and screen backlight. 

    `TWI` (I2C) is used to talk to the RPi. When AVR wants attention, the AVR_IRQ pin is pulled low (otherwise it is left floating as RPI pulls it high). A fourth wire, RPI_POWEROFF is used to notify the AVR that RPi's power can be safely shut down. As per the gpio-poweroff overlay, once RPI is ready to be shutdown, it will drive the pin high for 100ms, then low and then high again. Shutdown is possible immediately after the pin goes up. 

    `RPI_EN` must be pulled high to cut the power to RPI, Radio, screen, light sensor, thumbstick and rumbler off. The power is on by default (pulled low externally) so that RPi can be used to re-program the AVR via I2C and to ensure that RPi survives any possible AVR crashes. 

    `RGB_EN` controls a RGB led's power (it takes about 1mA when on even if dark). Its power source is independednt so that it can be used to signal error conditions such as low battery, etc. 

 */
class RCKid {
public:
    static constexpr gpio::Pin VBATT = 0; // ADC0, channel 4
    static constexpr gpio::Pin MIC_OUT = 1; // ADC1, channel 1
    static constexpr gpio::Pin BTNS_2 = 2; // ADC0, channel 6
    static constexpr gpio::Pin BTNS_1 = 3; // ADC0, channel 7
    static constexpr gpio::Pin JOY_V = 4; // ADC0, channel 8
    static constexpr gpio::Pin JOY_H = 5; // ADC0, channel 9
    static constexpr gpio::Pin RGB_EN = 6; // digital, floating
    static constexpr gpio::Pin RPI_EN = 7; // digital, floating
    static constexpr gpio::Pin CHARGE = 8; // ADC0, channel 10

    static constexpr gpio::Pin VIB_EN = 16; // TCA W3
    static constexpr gpio::Pin SCL = 15; // I2C alternate
    static constexpr gpio::Pin SDA = 14; // I2C alternate
    static constexpr gpio::Pin BTN_HOME = 13; // digital
    static constexpr gpio::Pin RGB = 12; // digital
    static constexpr gpio::Pin RPI_POWEROFF = 11; // analog input, ADC1 channel 7
    static constexpr gpio::Pin AVR_IRQ = AVR_PIN_AVR_IRQ; // 10, digital 
    static constexpr gpio::Pin BACKLIGHT = AVR_PIN_BACKLIGHT; // 9,  TCA W0

    /** \name State
     */
    //@{

    // Device state
    static inline ExtendedState state_;

    // Extra flags that are not meant to be shared with RPi as much
    static inline struct {
        /// true if the end next tick should raise IRQ
        bool irq: 1;
        /// set when I2C command has been received
        bool i2cReady : 1;
        /// true if there is at least one valid batch of audio data
        bool validBatch : 1;
        /// RTC one second tick ready
        bool secondTick : 1;
        /// BTN_HOME pin change detected
        bool btnHome : 1;
        /// Critical battery has been reached - reset by charging above certain threshold 
        bool batteryCritical : 1;
    } flags_;

    //@}

    /** \name Initialization

        This is the power-on initialization routine. 
        
     */
    //@{

    static void initialize() __attribute__((always_inline)) {
        // enable 2 second watchdog so that the second tick resets it always with enough time to spare
        while (WDT.STATUS & WDT_SYNCBUSY_bm); // required busy wait
            _PROTECTED_WRITE(WDT.CTRLA, WDT_PERIOD_2KCLK_gc);                
        // set CLK_PER prescaler to 2, i.e. 10Mhz, which is the maximum the chip supports at voltages as low as 3.3V
        CCP = CCP_IOREG_gc;
        CLKCTRL.MCLKCTRLB = CLKCTRL_PEN_bm; 
        // ensure pins are floating if for whatever reason they would not be
        gpio::input(RPI_EN);
        gpio::input(AVR_IRQ);
        gpio::input(RGB_EN);
        gpio::input(BTN_HOME);
        gpio::input(RGB);
        gpio::input(BACKLIGHT); // do not leak voltage
        gpio::input(VIB_EN); // do not leak voltage
        // initialize the ADC connected pins for better performance (turn of pullups, digital I/O, etc.)
        static_assert(VBATT == 0); // PA4
        PORTA.PIN4CTRL &= ~PORT_ISC_gm;
        PORTA.PIN4CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTA.PIN4CTRL &= ~PORT_PULLUPEN_bm;
        static_assert(MIC_OUT == 1); // PA5
        PORTA.PIN5CTRL &= ~PORT_ISC_gm;
        PORTA.PIN5CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTA.PIN5CTRL &= ~PORT_PULLUPEN_bm;
        static_assert(BTNS_2 == 2); // PA6
        PORTA.PIN6CTRL &= ~PORT_ISC_gm;
        PORTA.PIN6CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTA.PIN6CTRL &= ~PORT_PULLUPEN_bm;
        static_assert(BTNS_1 == 3); // PA7
        PORTA.PIN7CTRL &= ~PORT_ISC_gm;
        PORTA.PIN7CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTA.PIN7CTRL &= ~PORT_PULLUPEN_bm;
        static_assert(JOY_V == 4); // PB5
        PORTB.PIN5CTRL &= ~PORT_ISC_gm;
        PORTB.PIN5CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTB.PIN5CTRL &= ~PORT_PULLUPEN_bm;
        static_assert(JOY_H == 5); // PB4
        PORTB.PIN4CTRL &= ~PORT_ISC_gm;
        PORTB.PIN4CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTB.PIN4CTRL &= ~PORT_PULLUPEN_bm;
        static_assert(CHARGE == 8); // PB1
        PORTB.PIN1CTRL &= ~PORT_ISC_gm;
        PORTB.PIN1CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTB.PIN1CTRL &= ~PORT_PULLUPEN_bm;
        static_assert(RPI_POWEROFF == 11); // PC1
        PORTC.PIN1CTRL &= ~PORT_ISC_gm;
        PORTC.PIN1CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTC.PIN1CTRL &= ~PORT_PULLUPEN_bm;
        // initialize the RTC that fires every second for a semi-accurate real time clock keeping on the AVR, also start the timer
        RTC.CLKSEL = RTC_CLKSEL_INT1K_gc; // select internal oscillator divided by 32
        RTC.PITINTCTRL |= RTC_PI_bm; // enable the interrupt
        RTC.PITCTRLA = RTC_PERIOD_CYC1024_gc | RTC_PITEN_bm;
        // initialize TCB1 for a 1ms interval so that we can have a millisecond timer for the user interface (can't use cpu::delay_ms as arduino's default implementation uses own timers)
        TCB1.CTRLB = TCB_CNTMODE_INT_gc;
        TCB1.CCMP = 5000; // for 1kHz, 1ms interval
        TCB1.INTCTRL = 0;
        TCB1.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
        // initialize TCA for the backlight and rumbler PWM without turning it on, remeber that for the waveform generator to work on the respective pins, they must be configured as output pins (!)
        TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm; // enable split mode
        TCA0.SPLIT.CTRLB = 0;    
        //TCA0.SPLIT.CTRLB = TCA_SPLIT_LCMP0EN_bm | TCA_SPLIT_HCMP0EN_bm; // enable W0 and W3 outputs on pins
        //TCA0.SPLIT.LCMP0 = 64; // backlight at 1/4
        //TCA0.SPLIT.HCMP0 = 128; // rumbler at 1/2
        TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV64_gc | TCA_SPLIT_ENABLE_bm; 
        // initialize the I2C in alternate position
        PORTMUX.CTRLB |= PORTMUX_TWI0_bm;
        // initialize flags as empty
        static_assert(sizeof(flags_) == 1);
        *((uint8_t*)(&flags_)) = 0;
        // initalize the i2c slave with our address
        i2c::initializeSlave(AVR_I2C_ADDRESS);
        // enable BTN_HOME interrupt and internal pull-up, invert the pin's value so that we read the button nicely
        static_assert(BTN_HOME == 13); // PC3
        PORTC.PIN3CTRL |= PORT_ISC_BOTHEDGES_gc | PORT_PULLUPEN_bm | PORT_INVEN_bm;
        // verify that wakeup conditions have been met, i.e. that we have enough voltage, etc. and go to sleep immediately if that is not the case. Repeat until we can wakeup. NOTE going to sleep here cuts the power to RPi immediately which can be harmful, but if we are powering on with low battery, there is not much else we can do and the idea is that this ends long time before the RPi gets far enough in the booting process to be able to actually cause an SD card damage  
        if (!canWakeUp())
            sleep();
        // wakeup checks have passed, switch to powerUp phase immediately (no waiting for the home button long press in the case of power on)
        setMode(Mode::PowerUp);
    }


    //@}

    /** \name Main Loop and Power Modes

        ## Wakeup

        The first mode in the power sequence after sleep. Entered immediately after AVR power on, or home button press.

        TODO arguably turning off power here is ok because RPi did not yet start writing to the SD card, so there should be no corruption (we are still deep in kernel startup after 2 seconds). Is this true? 

        ## PowerOn

        When the Home button has been pressed for long enough, the wake up mode transitions to power on mode during which 

        ## On

        This is the default mode during which the Pi is left to its own. AVR is true slave and will only do what RPi tells it. 

        ## PowerDown

        RPI shoukd turn itself off. When done (detected via the RPI_POWEROFF pin) the AVR transitions to the sleep state. If the RPI fails to turn itself off in time, the last error is set accordingly.  
    
     */
    //@{

    static inline uint16_t timeout_ = 0;

    /** The main loop function. Checks the flags, adc results, etc. 
     */
    static void loop() __attribute__((always_inline)) {
        // go to sleep if we should
        if (state_.status.mode() == Mode::Sleep)
            sleep();
        // if there is a command, process it 
        if (flags_.i2cReady) {
            processCommand();
            // be ready for next command to be received
            i2cBufferIdx_ = 0;
            flags_.i2cReady = false;
        }
        // check the ADC used for controls and peripherals, which is also used to measure the internal ticks
        if (adcRead()) {
            if (timeout_ > 0 && --timeout_ == 0)
                timeoutError();
            if (flags_.irq)
                setIrq();
            flags_.irq = false;
        }
        // second tick timekeeping and wdt
        if (flags_.secondTick) {
            secondTick();
            flags_.secondTick = false;
        }
        // check the home button state and presses
        if (flags_.btnHome) {
            btnHomeChange();
            flags_.btnHome = false;            
        }
        // check the RPI poweroff pin to determine if we can go to the sleep
        if (state_.status.mode() == Mode::PowerDown) {
            if (ADC1.INTFLAGS & ADC_RESRDY_bm) {
                if ((ADC1.RES / 64) >= 150) // > 2.9V at 5V VCC, or > 1.9V at 3V3, should be enough including some margin
                    state_.status.setMode(Mode::Sleep);
                else
                    ADC1.COMMAND = ADC_STCONV_bm;
            }
        }
    }

    /** Busy delays the given amount of milliseconds using the otherwise unused TCB1. The delay function also resets the watchdog ensuring that even large delays can be processes savely. 
     */
    static void delayMs(uint16_t value) {
        while (value > 0) {
            while (! TCB1.INTFLAGS & TCB_CAPT_bm);
            TCB1.INTFLAGS |= TCB_CAPT_bm;
            --value;
            wdt::reset();
        }
    }

    /** A one-second tick from the RTC, used for timekeeping. 
     */
    static void secondTick() {
        state_.time.secondTick();
        wdt::reset();
    }

    /** Called when there is a change in BTN_HOME. Releasing it in the WakeUp mode cancels the wakeup and goes to power down immediately, while during the On mode, we set the timeout to home button press at which point we transition to the power down state. 
     */
    static void btnHomeChange() {
        switch (state_.status.mode()) {
            case Mode::WakeUp:
                // a very crude form of deboucing
                if (gpio::read(BTN_HOME) == false)
                    setMode(Mode::PowerDown);
                break;
            case Mode::On:
            case Mode::PowerUp: // also in power on mode in case we are in debug mode
                if (gpio::read(BTN_HOME)) {
                    state_.controls.setButtonHome(true);
                    setTimeout(BTN_HOME_POWERON_PRESS);
                } else {
                    state_.controls.setButtonHome(false);
                    setTimeout(0);
                }
                flags_.irq = true;
                break;
            default:
                // don't do anything
                break;
        }
    }

    /** Sleep implementation. Turns everything off and goes to sleep. When sleeping only the BTN_HOME press and RTC second tick will wake the device up and both are acted on accordingly.  
     */
    static void sleep() {
        // clear irq
        gpio::input(AVR_IRQ);
        flags_.irq = false;
        // cut power to RPI
        gpio::output(RPI_EN);
        gpio::high(RPI_EN);
        // cut power to the RGB
        gpio::input(RGB_EN);
        // turn of ADCs
        ADC1.CTRLA = 0;
        ADC0.CTRLA = 0;
        // turn of backlight
        setBrightness(0);
        // turn off rumbler
        setRumbler(0);
        // sleep, we'll periodically wake up for second ticks and for btnHome press 
        while (true) {
            cpu::sleep();
            if (flags_.secondTick) {
                secondTick();
                flags_.secondTick = false;
            }
            if (flags_.btnHome && gpio::read(BTN_HOME)) {
                flags_.btnHome = false;
                if (canWakeUp())
                    break;
            }
        }
        setMode(Mode::WakeUp);
    }

    /** Checks if we can wakeup, i.e. if there is enough voltage. 
     */
    static bool canWakeUp() {
        // ensure interrupts are enabled
        sei();
        // enable the ADC but instead of real ticks, just measure the VCC 10 times
        adcStart();
        uint16_t avgVcc = 0;
        for (uint8_t i = 0; i < 10; ++i) {
            while (! (ADC0.INTFLAGS & ADC_RESRDY_bm));
            uint16_t vcc = ADC0.RES / 64;
            vcc = 110 * 512 / vcc;
            vcc = vcc * 2;
            avgVcc += vcc;
            ADC0.COMMAND = ADC_STCONV_bm;
        }
        avgVcc = avgVcc / 10;
        if (avgVcc >= BATTERY_THRESHOLD_CHARGED)
            flags_.batteryCritical = false;
        if (avgVcc <= BATTERY_THRESHOLD_CRITICAL) // power-on from empty battery
            flags_.batteryCritical = true;
        // if the battery flag is too low, flash the critical battery lights and go back to sleep
        if (flags_.batteryCritical) {
            criticalBatteryWarning();
            return false;
        }
        // otherwise we can power on 
        return true;
    }

    /** Enters the given mode. 
     */
    static void setMode(Mode mode) {
        switch (mode) {
            case Mode::On:
                setTimeout(0); // disable the timeout
                break;
            // for sleep, don't do anything, all will be handled by the main loop that will enter sleep immediately
            case Mode::Sleep:
                break;
            case Mode::WakeUp:
                gpio::input(RPI_EN);
                setTimeout(BTN_HOME_POWERON_PRESS);
                // if we have error condition show that on the RGB led
                if (state_.dinfo.errorCode() != ErrorCode::NoError) {
                    rgbOn();
                    showColor(errorCodeColor(state_.dinfo.errorCode()));
                }
                // reset the comms state for the power up
                setDefaultTxAddress();
                break;
            case Mode::PowerUp:
                // if any other than no-error state, or select button being presed as well, make display visible immediately
                if (state_.dinfo.errorCode() != ErrorCode::NoError || state_.controls.select())
                    setBrightness(128);
                // disable the timeout if Select button is pressed
                setTimeout(state_.controls.select() ? 0 : RPI_POWERUP_TIMEOUT);
                // rumble to indicate true power on and set the timeout for RPI poweron 
                rumblerOk();
                break;
            case Mode::PowerDown:
                stopRecording();
                setBrightness(0);
                setTimeout(RPI_POWERDOWN_TIMEOUT);
                // start ADC1 in normal mode, read RPI_POWEROFF pin, start the conversion
                ADC1.CTRLB = ADC_SAMPNUM_ACC64_gc; 
                ADC1.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
                ADC1.CTRLD = ADC_INITDLY_DLY32_gc;
                ADC1.SAMPCTRL = 0;
                ADC1.INTCTRL = 0; // no interrupts
                ADC1.MUXPOS = ADC_MUXPOS_AIN7_gc;
                ADC1.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_8BIT_gc;
                ADC1.COMMAND = ADC_STCONV_bm;
                break;
            default:
                break;
        }
        state_.status.setMode(mode);
    }

    /** Sets the timeout, measured in ticks. Settin the timeout to 0 disables the function. 
     */
    static void setTimeout(uint16_t value) {
        timeout_ = value;
    }

    /** Called when timeout (Rpi power on or power off occurs). Sets the latest error, its color, rumbles and then goes to sleep. 
     
        The timeout is also used for long BTN_HOME press during normal mode, we do rumbler fail, then go to power down mode immediately. The IRQ is set to inform the RPi of the mode transition so that it can react and turn itself off. 
     */
    static void timeoutError() {
        switch (state_.status.mode()) {
            case Mode::PowerUp:
                state_.dinfo.setErrorCode(ErrorCode::RPiBootTimeout);
                break;
            case Mode::PowerDown:
                state_.dinfo.setErrorCode(ErrorCode::RPiPowerDownTimeout);
                break;
            case Mode::On:
                rumblerFail();
                setMode(Mode::PowerDown);
                setIrq();
                return;
        }
        rgbOn();
        showColor(errorCodeColor(state_.dinfo.errorCode()));
        rumblerFail();
        setMode(Mode::Sleep);
    }

    /** Sets the LCD display brightness. Set 0 to turn the display off. 
     */
    static void setBrightness(uint8_t value) {
        if (value == 0) { // turn off
            TCA0.SPLIT.CTRLB &= ~TCA_SPLIT_LCMP0EN_bm;
            gpio::input(BACKLIGHT);
        } else {
            gpio::output(BACKLIGHT);
            TCA0.SPLIT.CTRLB |= TCA_SPLIT_LCMP0EN_bm;
            TCA0.SPLIT.LCMP0 = value;
        }
    }

    /** Converts the error code to a color. 
     */
    static Color errorCodeColor(ErrorCode e) {
        switch (e) {
            case ErrorCode::InitialPowerOn:
                return Color::White();
            case ErrorCode::WatchdogTimeout:
                return Color::Cyan();
            case ErrorCode::RPiBootTimeout:
                return Color::Blue();
            case ErrorCode::RPiPowerDownTimeout:
                return Color::Green();
            default:
                return Color::Black();
        }
    }

    //@}

    /** \name Communication with RPI
     
        Communication with RPI is done via I2C bus with AVR being the slave. Additionally, the AVR_IRQ pin is pulled high to 3V3 by the RPI and can be set low by AVR to indicate to RPI a state change.

        Master write operations simply write bytes in the i2c buffer. When master write is done, the `i2cReady` flag is set signalling to the main loop that data from master are available. The main loop then reads the i2c buffer and performs the received command, finally resetting the `i2cready` flag. Only one command can be processed at once and the AVR will return nack if a master write is to be performed while `i2cready` flag is high. 

        Master read always reads the first status byte first giving information about the most basic device state. This first byte is then followed by the bytes read from the `i2cTxAddress`. By default, this will continue reading the rest of the state, but depending on the commands and state can be redirected to other addresses such as the command buffer itself, or the audio buffer. When not in recording mode, after each master read the `i2cTxAddress` is reset to the default value sending more state.  

        When recording, the AVR will be sending the recording buffer data preceded by the state byte. However, in recording mode the mode bits of the status are used to 
     */
    //@{

    // The I2C buffer used to store incoming data
    static inline uint8_t i2cBuffer_[I2C_BUFFER_SIZE];
    static inline uint8_t i2cBufferIdx_ = 0;

    static constexpr uint8_t TX_START = 0xff;

    static inline uint8_t * i2cTxAddress_ = nullptr;
    static inline uint8_t i2cNumTxBytes_ = TX_START;

    /** Pulls the AVR_IRQ pin low indicating to the RPI to talk to the AVR. Cleared automatically by the I2C interrupt vector. 
     */
    static void setIrq() {
        gpio::output(AVR_IRQ);
        gpio::low(AVR_IRQ);
    }

    /** Sets the default tx address when the initial status byte is followed by the rest of the state. To do so, the tx address is set to the state's address + 1 to account for the already sent status byte.
     */
    static void setDefaultTxAddress() {
        i2cTxAddress_ = ((uint8_t *) (& state_)) + 1;
    }

    /** Sets the tx address to given value meaning that next buffer read from the device will start here. 
     */
    static void setTxAddress(uint8_t * address) {
        i2cTxAddress_ = address;
    }

    /** Handles the processing of a previously received I2C comand. 
     */
    static void processCommand() {
        using namespace msg;
        switch (i2cBuffer_[0]) {
            // nothing, as expected
            case Nop::ID: {
                break;
            }
            // resets the chip
            case AvrReset::ID: {
                _PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRE_bm);
                // unreachable here
            }
            // sends the information about the chip in the same way the bootloader does. The command first stores the information in the i2cCommand buffer and then sets the command buffer as the tx address
            case msg::Info::ID:
                i2cBuffer_[0] = SIGROW.DEVICEID0;
                i2cBuffer_[1] = SIGROW.DEVICEID1;
                i2cBuffer_[2] = SIGROW.DEVICEID2;
                i2cBuffer_[3] = 1; // app
                for (uint8_t i = 0; i < 10; ++i)
                    i2cBuffer_[4 + i] = ((uint8_t*)(&FUSE))[i];
                i2cBuffer_[15] = CLKCTRL.MCLKCTRLA;
                i2cBuffer_[16] = CLKCTRL.MCLKCTRLB;
                i2cBuffer_[17] = CLKCTRL.MCLKLOCK;
                i2cBuffer_[18] = CLKCTRL.MCLKSTATUS;
                i2cBuffer_[19] = MAPPED_PROGMEM_PAGE_SIZE >> 8;
                i2cBuffer_[20] = MAPPED_PROGMEM_PAGE_SIZE & 0xff;
                i2cTxAddress_ = (uint8_t *)(& i2cBuffer_);
                // let the RPi know we have processed the command
                setIrq();
                break;
            // starts the audio recording 
            case msg::StartAudioRecording::ID: {
                if (state_.status.mode() == Mode::On && state_.status.recording() == false)
                    startRecording();
                break;
            }
            // stops the recording
            case msg::StopAudioRecording::ID: {
                if (state_.status.recording())
                    stopRecording();
                break;
            }
            // sets the display brightness
            case SetBrightness::ID: {
                auto & m = SetBrightness::fromBuffer(i2cBuffer_);
                state_.einfo.setBrightness(m.value);
                setBrightness(m.value);
                break;
            }
            // sets the time
            case SetTime::ID: {
                auto & m = SetTime::fromBuffer(i2cBuffer_);
                cli(); // avoid corruption by the RTC interrupt
                state_.time = m.value;
                sei();
                break;
            }
            // sets the alarm time
            case SetAlarm::ID: {
                auto & m = SetAlarm::fromBuffer(i2cBuffer_);
                state_.alarm = m.value;
                break;
            }
            case RumblerOk::ID: {
                rumblerOk();
                break;
            }
            case RumblerFail::ID: {
                rumblerFail();
                break;
            }
            case Rumbler::ID: {
                auto & m = Rumbler::fromBuffer(i2cBuffer_);
                setRumbler(m.intensity);
                // TODO deal with the timeout
                break;
            }


            case PowerOn::ID: {
                if (state_.status.mode() == Mode::PowerUp)
                    setMode(Mode::On);
                break;
            }
            case PowerDown::ID: {
                if (state_.status.mode() == Mode::On || state_.status.mode() == Mode::PowerUp)
                    setMode(Mode::PowerDown);
                break;
            }

        }
    }

    /** The I2C interrupt handler. 
     */
    static inline void TWI0_TWIS_vect() __attribute__((always_inline)) {
        #define I2C_DATA_MASK (TWI_DIF_bm | TWI_DIR_bm) 
        #define I2C_DATA_TX (TWI_DIF_bm | TWI_DIR_bm)
        #define I2C_DATA_RX (TWI_DIF_bm)
        #define I2C_START_MASK (TWI_APIF_bm | TWI_AP_bm | TWI_DIR_bm)
        #define I2C_START_TX (TWI_APIF_bm | TWI_AP_bm | TWI_DIR_bm)
        #define I2C_START_RX (TWI_APIF_bm | TWI_AP_bm)
        #define I2C_STOP_MASK (TWI_APIF_bm | TWI_DIR_bm)
        #define I2C_STOP_TX (TWI_APIF_bm | TWI_DIR_bm)
        #define I2C_STOP_RX (TWI_APIF_bm)
        ENTER_IRQ;
        uint8_t status = TWI0.SSTATUS;
        // sending data to accepting master is on our fastpath and is checked first. For the first byte we always send the status, followed by the data located at the txAddress. It is the responsibility of the master to ensure that only valid data sizes are reqested. 
        if ((status & I2C_DATA_MASK) == I2C_DATA_TX) {
            if (i2cNumTxBytes_ == TX_START)
                TWI0.SDATA = * (uint8_t*) (& state_.status);
            else
                TWI0.SDATA = i2cTxAddress_[i2cNumTxBytes_];
            TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
            ++i2cNumTxBytes_;
        // a byte has been received from master. Store it and send either ACK if we can store more, or NACK if we can't store more
        } else if ((status & I2C_DATA_MASK) == I2C_DATA_RX) {
            i2cBuffer_[i2cBufferIdx_++] = TWI0.SDATA;
            TWI0.SCTRLB = (i2cBufferIdx_ == sizeof(i2cBuffer_)) ? TWI_SCMD_COMPTRANS_gc : TWI_SCMD_RESPONSE_gc;
        // master requests slave to write data, reset the sent bytes counter, initialize the actual read address from the read start and reset the IRQ
        } else if ((status & I2C_START_MASK) == I2C_START_TX) {
            gpio::input(AVR_IRQ);
            TWI0.SCTRLB = TWI_ACKACT_ACK_gc + TWI_SCMD_RESPONSE_gc;
            flags_.validBatch = (wrIndex_ >> 5) != state_.status.batchIndex();
            // reset the watchdog
            __asm__ __volatile__ ("wdr\n");
        // master requests to write data itself. ACK if there is no pending I2C message, NACK otherwise. The buffer is reset to 
        } else if ((status & I2C_START_MASK) == I2C_START_RX) {
            TWI0.SCTRLB = flags_.i2cReady ? TWI_ACKACT_NACK_gc : TWI_SCMD_RESPONSE_gc;
        // sending finished, reset the tx address and when in recording mode determine if more data is available
        } else if ((status & I2C_STOP_MASK) == I2C_STOP_TX) {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
            if (! state_.status.recording()) {
                setDefaultTxAddress();
            } else if (i2cNumTxBytes_ == 32 && flags_.validBatch) {
                uint8_t nextBatch = (state_.status.batchIndex() + 1) & 7;
                state_.status.setBatchIndex(nextBatch);
                setTxAddress(recBuffer_ + (nextBatch << 5));
                // if the next batch is already available, set the IRQ
                if (nextBatch != (wrIndex_ >> 5))
                    setIrq();
            }
            // reset the watchdog timeout in the poweron state
            //if (RCKid::state_.status.mode() == Mode::On)
            //    RCKid::setTimeout(RPI_PING_TIMEOUT);
            i2cNumTxBytes_ = TX_START;
        // receiving finished, inform main loop we have message waiting
        } else if ((status & I2C_STOP_MASK) == I2C_STOP_RX) {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
            flags_.i2cReady = true;
        } else {
            // error - a state we do not know how to handle
        }
        LEAVE_IRQ;
    }

    //@}

    /** \name Controls & Battery & Charging monitoring & Ticks

        `ADC0` handles all the analog inputs and their periodic reading. Since we need to change the MUXPOS after each reading, the reading is done in a polling mode. When the ADC cycles through all of its inputs, a tick is initiated (roughly at 200Hz), during which the real values are computed from the raw readings and appropriate action is taken (or the RPI is notified). 

        The ADC cycles through the following measurements: 

        - VCC for critical battery warning
        - TEMP
        - VBATT
        - CHARGE
        - BTNS_2
        - BTNS_1
        - JOY_H
        - JOY_V  

        `BTNS_1` and `BTNS_2` are connected to a custom voltage divider that allows us to sample multiple presses of 3 buttons using a single pin. The ladder assumes a 8k2 resistor fro VCC to common junction that is beaing read and is connected via the three buttons and three different resistors (8k2, 15k, 27k) to ground.  
     */
    //@{

    /** Starts the measurements on ADC0. 
    */
    static void adcStart() {
        // voltage reference to 1.1V (internal for the temperature sensor)
        VREF.CTRLA &= ~ VREF_ADC0REFSEL_gm;
        VREF.CTRLA |= VREF_ADC0REFSEL_1V1_gc;
        // delay 32us and sampctrl of 32 us for the temperature sensor, do averaging over 64 values, full precission
        ADC0.CTRLB = ADC_SAMPNUM_ACC64_gc;
        ADC0.MUXPOS = ADC_MUXPOS_INTREF_gc;
        ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm; // use VDD as reference for VCC sensing, 1.25MHz
        ADC0.CTRLD = ADC_INITDLY_DLY32_gc;
        ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc;
         // start new conversion
        ADC0.COMMAND = ADC_STCONV_bm;
    }

    static bool adcRead() {
        // if ADC is not ready, return immediately without a tick
        if (! (ADC0.INTFLAGS & ADC_RESRDY_bm))
            return false;
        uint16_t value = ADC0.RES / 64;
        uint8_t muxpos = ADC0.MUXPOS;
        // do stuff depending on what the ADC was doing, first move to the next ADC read and start the conversion, then process the current one
        switch (muxpos) {
            // VCC
            case ADC_MUXPOS_INTREF_gc:
                ADC0.MUXPOS = ADC_MUXPOS_TEMPSENSE_gc;
                ADC0.SAMPCTRL = 31;
                ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_INTREF_gc | ADC_SAMPCAP_bm; // internal reference
                break;
            // Temp
            case ADC_MUXPOS_TEMPSENSE_gc:
                ADC0.MUXPOS = ADC_MUXPOS_AIN4_gc;
                ADC0.SAMPCTRL = 0;
                ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm; 
                break;
            // VBATT
            case ADC_MUXPOS_AIN4_gc:
                ADC0.MUXPOS = ADC_MUXPOS_AIN10_gc;
                break;
            // CHARGE 
            case ADC_MUXPOS_AIN10_gc:
                ADC0_MUXPOS = ADC_MUXPOS_AIN6_gc;
                break;
            // BTNS_2 
            case ADC_MUXPOS_AIN6_gc:
                ADC0_MUXPOS = ADC_MUXPOS_AIN7_gc;
                break;
            // BTNS_1 
            case ADC_MUXPOS_AIN7_gc:
                ADC0_MUXPOS = ADC_MUXPOS_AIN8_gc;
                break;
            // JOY_V 
            case ADC_MUXPOS_AIN8_gc:
                ADC0_MUXPOS = ADC_MUXPOS_AIN9_gc;
                break;
            // JOY_H 
            case ADC_MUXPOS_AIN9_gc: // fallthrough to default case
            /** Reset the ADC to take VCC measurements */
            default:
                ADC0.MUXPOS = ADC_MUXPOS_INTREF_gc;
                ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm; // use VDD as reference for VCC sensing
                ADC0.SAMPCTRL = 0;
                break;
        }
        // start the next conversion
        ADC0.COMMAND = ADC_STCONV_bm;
        // process the last measurement while reading the next measurement
        switch (muxpos) {
            // convert the reading to voltage and update the state 
            case ADC_MUXPOS_INTREF_gc:
                value = 110 * 512 / value;
                value = value * 2;
                state_.einfo.setVcc(value);
                // if we have critical battery threshold go to powerdown mode immediately, set the battery critical flag
                if (value <= BATTERY_THRESHOLD_CRITICAL && ! flags_.batteryCritical) {
                    flags_.irq = true;
                    flags_.batteryCritical = true;
                    criticalBatteryWarning();
                    setMode(Mode::PowerDown);
                }
                flags_.irq = state_.status.setUsb(value >= VCC_THRESHOLD_VUSB) | flags_.irq;
                flags_.irq = state_.status.setLowBatt(value <= BATTERY_THRESHOLD_LOW) | flags_.irq;
                break;
            // convert temperature reading to temperature, the code is taken from the ATTiny datasheet example
            case ADC_MUXPOS_TEMPSENSE_gc: {
                int8_t sigrow_offset = SIGROW.TEMPSENSE1; 
                uint8_t sigrow_gain = SIGROW.TEMPSENSE0;
                int32_t t = value - sigrow_offset; // Result might overflow 16 bit variable (10bit+8bit)
                t *= sigrow_gain;
                // temp is now in kelvin range, to convert to celsius, remove -273.15 (x256)
                t -= 69926;
                // and now loose precision to 0.5C (x10, i.e. -15 = -1.5C)
                value = (t >>= 7) * 5;
                state_.einfo.setTemp(value);
                break;
            }
            // VBATT
            case ADC_MUXPOS_AIN4_gc:
                // convert the battery reading to voltage. The battery reading is relative to vcc, which we already have
                // TODO will this work? when we run on batteries, there might be a voltage drop on the switch? 
                value = value * 64; 
                // TODO
                state_.einfo.setVBatt(value);
                break;
            // CHARGE 
            case ADC_MUXPOS_AIN10_gc:
                value = (value >> 2) & 0xff;
                flags_.irq = state_.status.setCharging(value < 32) | flags_.irq;
                break;
            // BTNS_2 
            case ADC_MUXPOS_AIN6_gc:
                flags_.irq = state_.controls.setButtons2(decodeAnalogButtons((value >> 2) & 0xff)) | flags_.irq;
                break;
            // BTNS_1 
            case ADC_MUXPOS_AIN7_gc:
                flags_.irq = state_.controls.setButtons1(decodeAnalogButtons((value >> 2) & 0xff)) | flags_.irq;
                flags_.irq = state_.controls.setButtonHome(gpio::read(BTN_HOME)) | flags_.irq;
                break;
            // JOY_V 
            case ADC_MUXPOS_AIN8_gc:
                value = (value >> 2) & 0xff;
                flags_.irq = state_.controls.setJoyV(value) | flags_.irq;
                break;
            // JOY_H 
            case ADC_MUXPOS_AIN9_gc:
                value = (value >> 2) & 0xff;
                flags_.irq = state_.controls.setJoyH(value) | flags_.irq;
                return true;
        }
        return false;
    }

    /** Decodes the raw analog value read into the states of three buttons returned as LSB bits. The analog value is a result of a custom voltage divider ladder so that simultaneous button presses can be detected.
     */
    static uint8_t decodeAnalogButtons(uint8_t raw) {
        if (raw <= 94)
            return 0b111; 
        if (raw <= 105)
            return 0b110;
        if (raw <= 118)
            return 0b101;
        if (raw <= 132)
            return 0b100;
        if (raw <= 150)
            return 0b011;
        if (raw <= 179)
            return 0b010;
        if (raw <= 225)
            return 0b001;
        return 0;
    }

    //@}

    /** \name Audio Recording
      
        When audio recording is enabled, the ADC1 is started in a continuous mode that collects as many of the readings as possible. When the TCB0 fires (8kHz), the accumulated samples are averaged and stored to a circular buffer. The buffer is divided into 8 32byte long batches. Each time there is 32 sampled bytes available, the AVR_IRQ is set informing the RPi to read the batch.  

        While in recording mode, when RPi starts reading, a status byte is sent first, followed by the recording buffer contents. The status byte contains a flag that the AVR is in recording mode and a batch index that will be returned. When 32 bytes after the status byte are read and the batch currently being read has been valid at the beginning of the read sequence (i.e. the whole batch was sent), the batch index is incremented. If the next batch is also available in full, the AVR_IRQ is set, otherwise it will be set when the recorder crosses the batch boundary. 

        The 8 batches and AVR_IRQ combined should allow enough time for the RPi to be able to read and buffer the data as needed without skipping any batches - but if a skip occurs the batch index in the status byte should be enough to detect it. 
    */
    //@{

    /// This is where we accumulate the recorder samples
    static inline uint16_t micAcc_ = 0;
    /// Number of samples accumulated so far
    static inline uint8_t micSamples_ = 0;
    /// Circular buffer for the mic recorder, conveniently wrapping at 256, giving us 8 32 bytes long batches
    static inline uint8_t recBuffer_[256];
    /// Write index to the circular buffer
    static inline uint8_t wrIndex_ = 0;

    static void startRecording() {
        wrIndex_ = 0;
        state_.status.setRecording(true);
        setTxAddress(recBuffer_);
        // initialize ADC1
        ADC1.CTRLA = 0; // disable ADC so that any pending read from the main app is cancelled
        // TODO set reference voltage to something useful, such as 2.5? 
        ADC1.MUXPOS = ADC_MUXPOS_AIN1_gc;
        ADC1.INTCTRL |= ADC_RESRDY_bm;
        ADC1.CTRLB = ADC_SAMPNUM_ACC4_gc; 
        ADC1.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
        ADC1.CTRLD = 0; // no sample delay, no init delay
        ADC1.SAMPCTRL = 0;
        ADC1.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_8BIT_gc | ADC_FREERUN_bm;
        ADC1.COMMAND = ADC_STCONV_bm;
        // start the 8kHz timer on TCB0
        TCB0.CTRLB = TCB_CNTMODE_INT_gc;
        TCB0.CCMP = 625; // for 8kHz
        TCB0.INTCTRL = TCB_CAPT_bm;
        TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
    }

    static void stopRecording() {
        // restore the mode
        TCB0.CTRLA = 0;
        ADC1.CTRLA = 0;
        ADC1.INTCTRL = 0;
        // restore mode and then disable the recording mode, order is important as otherwise we might read the recording batch index and interpret it as mode
        state_.status.setMode(Mode::On);
        state_.status.setRecording(false);
    }

    /** Critical code, just accumulate the sampled value.
     */
    static inline void ADC1_RESRDY_vect(void) __attribute__((always_inline)) {
        ENTER_IRQ;
        micAcc_ += ADC1.RES;
        micSamples_ += 4; // we do four samples per interrupt
        LEAVE_IRQ;
    }

    /** 
     */
    static inline void TCB0_INT_vect(void) __attribute__((always_inline)) {
        ENTER_IRQ;
        TCB0.INTFLAGS = TCB_CAPT_bm;
        recBuffer_[wrIndex_++] = (micSamples_ > 0) ? (micAcc_ / micSamples_) : 0;
        micAcc_ = 0;
        micSamples_ = 0;
        // if we have accumulated a batch of readings, set the IRQ, do not change the batch index and whether we have a valid batch as this is always determined by the I2C routine when slave tx starts/ends.
        if (wrIndex_ % 32 == 0)
            setIrq();
        LEAVE_IRQ;
    }


    //@}

    /** \name Rumbler 
     */
    //@{

    static void rumblerOk() {
        setRumbler(128);
        delayMs(750);
        setRumbler(0);
    }

    static void rumblerFail() {
        for (int i = 0; i < 3; ++i) {
            setRumbler(128); // TODO go back to 255
            delayMs(100);
            setRumbler(0);
            delayMs(230);
        }
    }

    static void setRumbler(uint8_t value) {
        if (value == 0) { // turn off
            TCA0.SPLIT.CTRLB &= ~TCA_SPLIT_HCMP0EN_bm;
            gpio::input(VIB_EN);
        } else {
            gpio::output(VIB_EN);
            TCA0.SPLIT.CTRLB |= TCA_SPLIT_HCMP0EN_bm;
            TCA0.SPLIT.HCMP0 = 255 - value;
        }
    }

    //@}

    /** \name RGB Led and status information. 
     */
    //@{

    static inline NeopixelStrip<1> rgb_{RGB};

    static void rgbOn() {
        gpio::output(RGB);
        gpio::low(RGB);
        delayMs(10);
    }

    /** Turns the RGB led off. 
     */
    static void rgbOff() {
        gpio::input(RGB);
    }

    // TODO add timeout
    static void showColor(Color c) {
        rgb_.fill(c);
        rgb_.update(true);
    }


    /** Flashes the critical battery warning, 5 short red flashes. 
     */
    static void criticalBatteryWarning() {
        rgbOn();
        for (int i = 0; i < 5; ++i) {
            rgb_.fill(Color::Red());
            rgb_.update();
            delayMs(100);
            rgb_.fill(Color::Black());
            rgb_.update();
            delayMs(100);
        }
        rgbOff();
    }

    //@}




}; // RCKid

/** Interrupt handler for BTN_HOME (PC3), rising and falling.
 */
ISR(PORTC_PORT_vect) { 
    ENTER_IRQ;
    VPORTC.INTFLAGS |= (1 << 3); // PC3
    RCKid::flags_.btnHome = true;
    LEAVE_IRQ;
}

/** The RTC one second interval tick ISR. 
 */
ISR(RTC_PIT_vect) {
    ENTER_IRQ;
    RTC.PITINTFLAGS = RTC_PI_bm; // clear the interrupt
    RCKid::flags_.secondTick = true;
    LEAVE_IRQ;
}

ISR(TWI0_TWIS_vect) { 
    ENTER_IRQ;
    RCKid::TWI0_TWIS_vect(); 
    LEAVE_IRQ;
}

ISR(ADC1_RESRDY_vect) {
    ENTER_IRQ; 
    RCKid::ADC1_RESRDY_vect();
    LEAVE_IRQ;
}

ISR(TCB0_INT_vect) { 
    ENTER_IRQ;
    RCKid::TCB0_INT_vect();
    LEAVE_IRQ;
}

void setup() { RCKid::initialize(); }
void loop() { RCKid::loop(); }



