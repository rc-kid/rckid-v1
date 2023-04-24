#include <Arduino.h>

#include "platform/platform.h"
#include "platform/peripherals/neopixel.h"

#include "common/comms.h"
#include "common/config.h"



using namespace platform;
using namespace comms;

/** DEBUG: A very simple test that just makes the clock frequencies observable. It is not to be used when AVR is soldered on the RCKid board as it enables the CLKOUT on PB5 and reconfigures other pins. 
 */
#define TEST_CLOCK_

/** DEBUG: When enabled, the AVR will use the I2C bus in a master mode and will communicate with an OLED screen attached to it to display various statistics. DISABLE IN PRODUCTION
 */
#define TEST_I2C_DISPLAY_
#if (defined TEST_I2C_DISPLAY)
#include "platform/peripherals/ssd1306.h"
SSD1306 display;
#endif

#define ENTER_IRQ //gpio::high(RCKid::RGB)
#define LEAVE_IRQ //gpio::low(RCKid::RGB)


/** RCKid AVR Controller
 
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
    static constexpr gpio::Pin AVR_IRQ = 10; // digital 
    static constexpr gpio::Pin BACKLIGHT = 9; // TCA W0

    static void initialize() {
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
        // enable BTN_HOME interrupt and internal pull-up, invert the pin's value so that we read the button nicely
        static_assert(BTN_HOME == 13); // PC3
        PORTC.PIN3CTRL |= PORT_ISC_BOTHEDGES_gc | PORT_PULLUPEN_bm | PORT_INVEN_bm;
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
        *((uint8_t*)(&flags_)) = 0;
#if (!defined TEST_I2C_DISPLAY)
        // initalize the i2c slave with our address
        i2c::initializeSlave(AVR_I2C_ADDRESS);
#else
        i2c::initializeMaster();
        display.initialize128x32();
        display.clear32();
        display.write(0, 0, "  :  :");
        display.write(0, 1, "VCC: ");
        display.write(0, 2, "Temp:");
        display.write(0, 3, "VBatt:");
#endif
        // verify that wakeup conditions have been met, i.e. that we have enough voltage, etc. and go to sleep immediately if that is not the case. Repeat until we can wakeup. NOTE going to sleep here cuts the power to RPi immediately which can be harmful, but if we are powering on with low battery, there is not much else we can do and the idea is that this ends long time before the RPi gets far enough in the booting process to be able to actually cause an SD card damage  
        while (!wakeUp())
            sleep();
        // wakeup checks have passed, switch to powerUp phase immediately (no waiting for the home button long press in the case of power on)
        powerUp();
    }

    static void loop() {
        switch (state_.status.mode()) {
            // all the sleep logic is in the sleep function, we simply call it from the main loop here
            case Mode::Sleep:
                sleep();
                break;
            // we have to check two things during the wake up mode, if the home button has been released before the timeout, then this was fake wakeup, and we go to powering down mode immediately. If the timeout has been reached, it means the pres was long enough and we switch to PowerUp mode. If wakeup occurs in the repairMode, deal with repair mode instead. 
            case Mode::WakeUp:
                // if the select has been released (or never pressed), disable transition to repair mode
                if (!state_.controls.select())
                    flags_.manualRepairMode = false;
                if (state_.dinfo.repairMode())
                    repairMode();
                if (flags_.countdownTimeout)
                    powerUp();
                else if (flags_.btnHome)
                    powerDown();
                break;
            // monitor the timeout flag, if triggered, then RPi failed to start properly in the specified time and we must react
            case Mode::PowerUp:
                if (flags_.countdownTimeout)
                    rpiBootTimeout();
                break;
            // in power on, monitor the ping timeout, if triggered, do a RPI restart.  
            case Mode::On:
                if (flags_.countdownTimeout)
                    rpiPingTimeout();
                break;
            // in power down, monitor the RPI_POWEROFF line to see when it gets high and so its safe to turn the RPi off. If the timeout is triggered, force power off.  
            case Mode::PowerDown:
                if (ADC1.INTFLAGS & ADC_RESRDY_bm) {
                    if ((ADC1.RES / 64) >= 150) // > 2.9V at 5V VCC, or > 1.9V at 3V3, should be enough including some margin
                        state_.status.setMode(Mode::Sleep);
                    else
                        ADC1.COMMAND = ADC_STCONV_bm;
                }
                // TODO keep the error somewhere or some such
                if (flags_.countdownTimeout)
                    rpiPowerDownTimeout();
                break;
        }
        // in all modes, process any received I2C commands, ADC0 results and check second tick flag. If the ADC goes full circle, perform tick as well
        if (flags_.i2cReady)
            processCommand();
        if (adcReady())
            tick();
        if (flags_.secondTick)
            secondTick();
        // home button long press detection (we use the tick method and its regular sampling of the button pins to inform the RPi of the event. This code is solely for the long press to power off detection. 
        if (flags_.btnHome) {
            flags_.btnHome = false;
            if (gpio::read(BTN_HOME))
                btnHomeLongPressTimeout_ = BTN_HOME_POWEROFF_PRESS;
            setIrq(); // set IRQ since btn home has been pressed/release and it might just be a short UI press
        }
    }

    static void secondTick() {
        flags_.secondTick = false;
        state_.time.secondTick();
#if (defined TEST_I2C_DISPLAY)
        display.writeNumber(0, 0, state_.time.hour(), 2, '0');
        display.writeNumber(15, 0, state_.time.minute(), 2, '0');
        display.writeNumber(30, 0, state_.time.second(), 2, '0');
        if (state_.status.mode() != Mode::Sleep) {
            display.write(35, 1, state_.einfo.vcc(), ' ');
            display.write(35, 2, state_.einfo.temp(), ' ');
            display.write(35, 3, state_.einfo.vbatt(), ' ');
        }
#endif
    }


    /** Clears the timeout flag and sets a new timeout value. 
     */
    static void setTimeout(uint16_t value) {
        flags_.countdownTimeout = false;
        countdownTicks_ = value;
    }

    /** Busy delays the given amount of milliseconds using the otherwise unused TCB1. 
     */
    static void delayMs(uint16_t value) {
        while (value > 0) {
            while (! TCB1.INTFLAGS & TCB_CAPT_bm);
            TCB1.INTFLAGS |= TCB_CAPT_bm;
            --value;
        }
    }

    /** \name State Transitions
     
        `PoweringDown`

        In this state the the RPi is powering itself down. We are waiting for high on the RPI_POWEROFF pin after which we can cut power to the RPi by driving the RPI_EN pin high and then going to sleep. 
     
    */
    //@{

    static inline uint16_t btnHomeLongPressTimeout_ = 0;

    static void btnHomePowerOff() {
        if (state_.status.mode() == Mode::On)
            powerDown();
    }

    /** Cuts power to the RPi and all other peripherals and puts the AVR to sleep. 
     */
    static void sleep() {
#if (defined TEST_I2C_DISPLAY)
        display.write(64, 1, "SLEEP");
#endif
        // cut power to RPI
        gpio::output(RPI_EN);
        gpio::high(RPI_EN);
        // cut power to the RGB
        gpio::input(RGB_EN);
        // turn of ADCs
        ADC1.CTRLA = 0;
        ADC0.CTRLA = 0;
        // TODO turn off backlight & rumbler

#if (defined TEST_I2C_DISPLAY)
        display.clear32();
#endif
        while (true) {
            wdt::disable();
            cpu::sleep();
            if (flags_.secondTick)
                secondTick();
            if (flags_.btnHome && gpio::read(BTN_HOME)) {
                flags_.btnHome = false;
                if (wakeUp())
                    break;
            }
        }
        // when we get here, we are leaving the power down mode, set status to have Home and Select button both pressed (Home because power on, select tentatively to detect manual transition to repair mode)
        state_.controls.setButtons(0, 1, true);
        flags_.manualRepairMode = true;
    }

    /** Enters the WakeUp state. Called when BTN_HOME is pressed in Sleep state. 
     */
    static bool wakeUp() {
#if (defined TEST_I2C_DISPLAY)
        display.write(64, 1, "WAKE_UP");
#endif
        // ensure interrupts are allowed
        sei();
        // enable the ADC but instead of real ticks, just measure the VCC 10 times making sure that it never gets below the critical battery threshold
        adcStart();
        int bad = 0;
        uint16_t avgVcc;
        for (uint8_t i = 0; i < 10; ++i) {
            while (! (ADC0.INTFLAGS & ADC_RESRDY_bm));
            uint16_t vcc = ADC0.RES / 64;
            vcc = 110 * 512 / vcc;
            vcc = vcc * 2;
            if (vcc <= BATTERY_THRESHOLD_CRITICAL)
                ++bad;
            avgVcc += vcc;
            ADC0.COMMAND = ADC_STCONV_bm;
        }
        // only fail to start if the voltage is really low. 
        if (bad > 3) {
            criticalBatteryWarning();
            return false;
        }
        if (state_.dinfo.repairMode()) {
            showColor(Color::Red());
            avgVcc = avgVcc / 10;
            if (avgVcc < VCC_THRESHOLD_VUSB) {
                rumblerFail();
                return false;
            }
        } else {
            switch (state_.dinfo.errorCode()) {
                case ErrorCode::RPiPowerDownTimeout:
                    showColor(Color::Blue());
                    break;
                case ErrorCode::RPiPingTimeout:
                    showColor(Color::Green());
                    break;
                default:
                    break;
            }
        }
        // we have ok voltage, turn on the rpi and get ready for normal operation, set the timer for the long power button press
        gpio::input(RPI_EN);
        setTimeout(BTN_HOME_POWERON_PRESS);
        state_.status.setMode(Mode::WakeUp);
        // reset the comms state for the power up
        setDefaultTxAddress();
        // and return success;
        return true; 
    }

    /** Enters the PowerUp mode. Fire the rumbler to signal we are powering on and set the timeout for the power up seuence. */
    static void powerUp() {
#if (defined TEST_I2C_DISPLAY)
        display.write(64, 1, "POWER_UP");
#endif
        rumblerOk();
        if (flags_.manualRepairMode) {
            repairMode();
        } else {
            flags_.countdownTimeout = false;
            setTimeout(RPI_POWERUP_TIMEOUT);
            state_.status.setMode(Mode::PowerUp);
        }
    }

    /** Enters the powering down mode. Turns on ADC1 on the RPI_POWEROFF pin where we wait for 3V3 high signal that informs the AVR that it can cut RPi's power safely. 
     */
    static void powerDown() {
#if (defined TEST_I2C_DISPLAY)
        display.write(64, 1, "POWER_DOWN");
#endif
        stopRecording();
        setTimeout(RPI_POWERDOWN_TIMEOUT);
        state_.status.setMode(Mode::PowerDown);
        // start ADC1 in normal mode, read RPI_POWEROFF pin, start the conversion
        ADC1.CTRLB = ADC_SAMPNUM_ACC64_gc; 
        ADC1.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
        ADC1.CTRLD = ADC_INITDLY_DLY32_gc;
        ADC1.SAMPCTRL = 0;
        ADC1.INTCTRL = 0; // no interrupts
        ADC1.MUXPOS = ADC_MUXPOS_AIN7_gc;
        ADC1.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_8BIT_gc;
        ADC1.COMMAND = ADC_STCONV_bm;
    }

    static void powerOn() {
#if (defined TEST_I2C_DISPLAY)
        display.write(64, 1, "ON  ");
#endif
        setTimeout(RPI_PING_TIMEOUT);
        state_.status.setMode(Mode::On);
    }

    /** Called when the RPi does not boot in the set time. Three retries are available, after which the device enters the repair mode and turns itself off. 
     */
    static void rpiBootTimeout() {
        flags_.countdownTimeout = false;
        // turn off RPI
        gpio::output(RPI_EN);
        gpio::high(RPI_EN);
        // set the debug info and error state
        if (state_.dinfo.errorCode() != ErrorCode::RPiBootTimeout)
            state_.dinfo.setErrorCode(ErrorCode::RPiBootTimeout);
        else
            state_.dinfo.setRepairMode(true);
        if (state_.dinfo.repairMode()) {
            showColor(Color::Red());
            rumblerFail();
            state_.status.setMode(Mode::Sleep);
        } else {
            showColor(Color::Green());
            setTimeout(RPI_POWERUP_TIMEOUT * 2);
            rumblerFail();
            gpio::input(RPI_EN);
        }
    }

    /** Called when the RPI times out during power down (i.e. we do not see the transition to high on RPI_POWEROFF pin). We simply set the error code and go to sleep immediately. 
    */
    static void rpiPowerDownTimeout() {
        flags_.countdownTimeout = false;
        if (state_.dinfo.repairMode())
            return;
        rumblerFail();
        state_.status.setMode(Mode::Sleep);
        state_.dinfo.setErrorCode(ErrorCode::RPiPowerDownTimeout);
    }

    /** Called when the RPI fails to talk to the AVR in a timely manner indicating errors with the RPi application. As this means that the RPi has effectively become unresponsible, we set the error and enter sleep mode. 
     */
    static void rpiPingTimeout() {
        flags_.countdownTimeout = false;
        if (state_.dinfo.repairMode())
            return;
        rumblerFail();
        state_.status.setMode(Mode::Sleep);
    }

    /** Repair mode when we have trouble entering booting up the RPi. 
     
        Since the RPi might not be able to it itself, set the brightness to max here to make the boot process possibly observable. 
     */
    static void repairMode() {
#if (defined TEST_I2C_DISPLAY)
        display.write(64, 1, "WAKE_UP");
#endif
        state_.status.setMode(Mode::On);
        showColor(Color::Red());
        setBrightness(255);
    }

    //}@

    /** \name ADC0 and ticks 

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

    static inline uint16_t countdownTicks_ = 0; 

    static inline uint16_t vcc_;
    static inline uint16_t temp_;
    static inline uint16_t vBatt_;
    static inline uint8_t charge_;
    static inline uint8_t btns2_;
    static inline uint8_t btns1_;
    static inline uint8_t joyH_;
    static inline uint8_t joyV_;

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

    /** Checks if the ADC0 is ready and processes its output if so. Returns true if the ADC has cycled through the entire list. 
      */
    static bool adcReady() {
        // if ADC is not ready, return immediately
        if (! (ADC0.INTFLAGS & ADC_RESRDY_bm))
            return false;
        bool isTick = false;
        uint16_t value = ADC0.RES / 64;
        switch (ADC0.MUXPOS) {
            // for the VCC we simply store the calculated value and move on to the temperature measurement. 
            case ADC_MUXPOS_INTREF_gc:
                vcc_ = value;
                ADC0.MUXPOS = ADC_MUXPOS_TEMPSENSE_gc;
                ADC0.SAMPCTRL = 31;
                ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_INTREF_gc | ADC_SAMPCAP_bm; // internal reference
                break;
            // simply stores the temperature reading and moves to the next measurement
            case ADC_MUXPOS_TEMPSENSE_gc:
                temp_ = value;
                ADC0.MUXPOS = ADC_MUXPOS_AIN4_gc;
                ADC0.SAMPCTRL = 0;
                ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm; 
                break;
            // VBATT
            case ADC_MUXPOS_AIN4_gc:
                vBatt_ = value;
                ADC0.MUXPOS = ADC_MUXPOS_AIN10_gc;
                break;
            // CHARGE 
            case ADC_MUXPOS_AIN10_gc:
                charge_ = (value >> 2) & 0xff;
                ADC0_MUXPOS = ADC_MUXPOS_AIN6_gc;
                break;
            // BTNS_2 
            case ADC_MUXPOS_AIN6_gc:
                btns2_ = (value >> 2) & 0xff;
                ADC0_MUXPOS = ADC_MUXPOS_AIN7_gc;
                break;
            // BTNS_1 
            case ADC_MUXPOS_AIN7_gc:
                btns1_ = (value >> 2) & 0xff;
                ADC0_MUXPOS = ADC_MUXPOS_AIN8_gc;
                break;
            // JOY_V 
            case ADC_MUXPOS_AIN8_gc:
                joyV_ = (value >> 2) & 0xff;
                ADC0_MUXPOS = ADC_MUXPOS_AIN9_gc;
                break;
            // JOY_H 
            case ADC_MUXPOS_AIN9_gc:
                joyH_ = (value >> 2) & 0xff;
                isTick = true;
                // fallthrough to default case
            /** Reset the ADC to take VCC measurements */
            default:
                ADC0.MUXPOS = ADC_MUXPOS_INTREF_gc;
                ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm; // use VDD as reference for VCC sensing
                ADC0.SAMPCTRL = 0;
                break;
        }
        // start new conversion
        ADC0.COMMAND = ADC_STCONV_bm;
        return isTick;
    }

    /** Called when the ADC cycles through all its measurements. Tick calculates the input controls and other ADC measurements and determines whether the RPi should be notified by the AVR_IRQ if there is a change. The function also performs more complex post processing to make sure the ADC is restarted as quickly as possible in the adcReady() function so that we do not waste cycles as the tick() function runs when the ADC is already busy.
     */
    static void tick() {
        // check the home button timeout 
        if (gpio::read(BTN_HOME) && (btnHomeLongPressTimeout_ > 0) && (--btnHomeLongPressTimeout_ == 0))
            btnHomePowerOff();
        // check the timeout, measured in ticks
        if (countdownTicks_ > 0 && --countdownTicks_ == 0)
            flags_.countdownTimeout = true;
        // convert vcc reading to voltage
        vcc_ = 110 * 512 / vcc_;
        vcc_ = vcc_ * 2;
        state_.einfo.setVcc(vcc_);
        // convert the battery reading to voltage. The battery reading is relative to vcc, which we already have
        // TODO will this work? when we run on batteries, there might be a voltage drop on the switch? 
        vBatt_ = vBatt_ * 64; 
        // TODO
        state_.einfo.setVBatt(vBatt_);
        // convert temperature reading to temperature, the code is taken from the ATTiny datasheet example
        int8_t sigrow_offset = SIGROW.TEMPSENSE1; 
        uint8_t sigrow_gain = SIGROW.TEMPSENSE0;
        int32_t t = temp_ - sigrow_offset; // Result might overflow 16 bit variable (10bit+8bit)
        t *= sigrow_gain;
        // temp is now in kelvin range, to convert to celsius, remove -273.15 (x256)
        t -= 69926;
        // and now loose precision to 0.5C (x10, i.e. -15 = -1.5C)
        temp_ = (t >>= 7) * 5;
        state_.einfo.setTemp(temp_);
        // get the events and controls and determine wherther to raise IRQ
        bool irq = false;
        irq = state_.status.setCharging(charge_ < 32) | irq;
        irq = state_.status.setUsb(vcc_ >= VCC_THRESHOLD_VUSB) | irq;
        irq = state_.status.setLowBatt(vcc_ <= BATTERY_THRESHOLD_LOW) | irq; 
        // decode the buttons and set the controls
        // TODO debounce? 
        uint8_t btns1 = decodeAnalogButtons(btns1_);
        uint8_t btns2 = decodeAnalogButtons(btns2_);
        irq = state_.controls.setButtons(btns1, btns2, gpio::read(BTN_HOME));
        // set the thumbstick position
        // TODO debounce? 
        irq = state_.controls.setJoyH(joyH_) | irq;
        irq = state_.controls.setJoyV(joyV_) | irq;
        // check the battery levels and take appropriate actions
        // TODO
        // if there has been a change in the irq, request IRQ
        if (irq)
            setIrq();

#if (defined TEST_I2C_DISPLAY)
        //display.write(35, 1, vcc_, ' ');
        //display.write(35, 2, temp_, ' ');
        //display.write(35, 3, vBatt_, ' ');
        display.write(64 + 35, 1, btns1);
        display.write(64 + 43, 1, btns2);
#endif
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

    /// this is where we accumulate the recorder samples
    static inline uint16_t micAcc_ = 0;
    // number of samples accumulated so far
    static inline uint8_t micSamples_ = 0;
    /// circular buffer for the mic recorder and the write index
    static inline uint8_t recBuffer_[256];
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
    //@}

    /** \name Communication with RPI
     
        The RPI communication works over the I2C connection and a decidated AVR_IRQ pin, which the RPi pulls high to 3V3 and the AVR outputs low when it wants to signal to the RPi.

        Any master-write operations simply store the data in the i2cBuffer which, when done is interpreted as an I2C command. All master-read operations send first the status byte itself, followed by the memory contents form an i2cTxAddress. By default, the i2cTxAddress points right after the status byte of the extended state, so that the whole state is returned, but it can be temporarily chaned by various I2C commands, or recording mode, in which case the appropriate batch from the recording buffer will be sent. 
     */
    //@{

    static constexpr uint8_t TX_START = 0xff;

    static inline uint8_t i2cBuffer_[I2C_BUFFER_SIZE];
    static inline uint8_t i2cBufferIdx_ = 0;

    static inline uint8_t * i2cTxAddress_ = nullptr;
    static inline uint8_t i2cNumTxBytes_ = TX_START;

    //@}

    static void setIrq() {
        // TODO ensure this actually outputs low and never high
        gpio::output(AVR_IRQ);
        gpio::low(AVR_IRQ);
    }

    /** Sets the default tx address when the initial status byte is followed by the rest of the state. To do so, the tx address is set to the state's address + 1 to account for the already sent status byte.
     */
    static void setDefaultTxAddress() {
        i2cTxAddress_ = ((uint8_t *) (& state_)) + 1;
    }

    static void setTxAddress(uint8_t * address) {
        i2cTxAddress_ = address;
    }

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
                    powerOn();
                break;
            }
            case PowerDown::ID: {
                if (state_.status.mode() == Mode::On || state_.status.mode() == Mode::PowerUp)
                    powerDown();
                break;
            }
            case EnterRepairMode::ID: {
                state_.dinfo.setRepairMode(true);
                repairMode();
                break;
            }
            case LeaveRepairMode::ID: {
                state_.dinfo.setRepairMode(false);
                powerOn();
                break;
            }

        }

        // be ready for next command to be received
        i2cBufferIdx_ = 0;
        flags_.i2cReady = false;
    }


    /** \name Outputs - RGB Light, rumbler and the backlight
     */
    //@{

    static inline NeopixelStrip<1> rgb_{RGB};

    static void criticalBatteryWarning() {
        gpio::output(RGB);
        delayMs(10);
        for (int i = 0; i < 5; ++i) {
            rgb_.fill(Color::Red());
            rgb_.update();
            delayMs(100);
            rgb_.fill(Color::Black());
            rgb_.update();
            delayMs(100);
        }
        gpio::input(RGB);
    }

    // TODO add timeout
    static void showColor(Color c) {
        gpio::output(RGB);
        gpio::low(RGB);
        rgb_.fill(c);
        rgb_.update(true);
    }

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

    //@}

    static inline ExtendedState state_;

    // TODO does this need to be volatile? 
    static inline struct {
        /// RTC one second tick ready
        bool secondTick : 1;
        /// BTN_HOME pin change detected
        bool btnHome : 1;
        /// set when countdown timeout reaches 0
        bool countdownTimeout : 1;
        /// set when I2C command has been received
        bool i2cReady : 1;
        /// true if there is at least one valid batch
        bool validBatch : 1;
        /// during wake-up, if this keeps true, we enter repair mode instead of power up. Cleared when SELECT is released before the end of wakeup cycle
        bool manualRepairMode : 1;
    } flags_;

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

/** Critical code, just accumulate the sampled value.
 */
ISR(ADC1_RESRDY_vect) {
    ENTER_IRQ;
    RCKid::micAcc_ += ADC1.RES;
    RCKid::micSamples_ += 4; // we do four samples per interrupt
    LEAVE_IRQ;
}

ISR(TCB0_INT_vect) {
    ENTER_IRQ;
    TCB0.INTFLAGS = TCB_CAPT_bm;
    RCKid::recBuffer_[RCKid::wrIndex_++] = (RCKid::micSamples_ > 0) ? (RCKid::micAcc_ / RCKid::micSamples_) : 0;
    RCKid::micAcc_ = 0;
    RCKid::micSamples_ = 0;
    // if we have accumulated a batch of readings, set the IRQ, do not change the batch index and whether we have a valid batch as this is always determined by the I2C routine when slave tx starts/ends.
    if (RCKid::wrIndex_ % 32 == 0)
        RCKid::setIrq();
    LEAVE_IRQ;
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
    ENTER_IRQ;
    uint8_t status = TWI0.SSTATUS;
    // sending data to accepting master is on our fastpath and is checked first. For the first byte we always send the status, followed by the data located at the txAddress. It is the responsibility of the master to ensure that only valid data sizes are reqested. 
    if ((status & I2C_DATA_MASK) == I2C_DATA_TX) {
        if (RCKid::i2cNumTxBytes_ == RCKid::TX_START)
            TWI0.SDATA = * (uint8_t*) (& RCKid::state_.status);
        else
            TWI0.SDATA = RCKid::i2cTxAddress_[RCKid::i2cNumTxBytes_];
        TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
        ++RCKid::i2cNumTxBytes_;
    // a byte has been received from master. Store it and send either ACK if we can store more, or NACK if we can't store more
    } else if ((status & I2C_DATA_MASK) == I2C_DATA_RX) {
        RCKid::i2cBuffer_[RCKid::i2cBufferIdx_++] = TWI0.SDATA;
        TWI0.SCTRLB = (RCKid::i2cBufferIdx_ == sizeof(RCKid::i2cBuffer_)) ? TWI_SCMD_COMPTRANS_gc : TWI_SCMD_RESPONSE_gc;
    // master requests slave to write data, reset the sent bytes counter, initialize the actual read address from the read start and reset the IRQ
    } else if ((status & I2C_START_MASK) == I2C_START_TX) {
        gpio::input(RCKid::AVR_IRQ);
        TWI0.SCTRLB = TWI_ACKACT_ACK_gc + TWI_SCMD_RESPONSE_gc;
        RCKid::flags_.validBatch = (RCKid::wrIndex_ >> 5) != RCKid::state_.status.batchIndex();
    // master requests to write data itself. ACK if there is no pending I2C message, NACK otherwise. The buffer is reset to 
    } else if ((status & I2C_START_MASK) == I2C_START_RX) {
        TWI0.SCTRLB = RCKid::flags_.i2cReady ? TWI_ACKACT_NACK_gc : TWI_SCMD_RESPONSE_gc;
    // sending finished, reset the tx address and when in recording mode determine if more data is available
    } else if ((status & I2C_STOP_MASK) == I2C_STOP_TX) {
        TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
        if (! RCKid::state_.status.recording()) {
            RCKid::setDefaultTxAddress();
        } else if (RCKid::i2cNumTxBytes_ == 32 && RCKid::flags_.validBatch) {
            uint8_t nextBatch = (RCKid::state_.status.batchIndex() + 1) & 7;
            RCKid::state_.status.setBatchIndex(nextBatch);
            RCKid::setTxAddress(RCKid::recBuffer_ + (nextBatch << 5));
            // if the next batch is already available, set the IRQ
            if (nextBatch != (RCKid::wrIndex_ >> 5))
                RCKid::setIrq();
        }
        // reset the watchdog timeout in the poweron state
        if (RCKid::state_.status.mode() == Mode::On)
            RCKid::setTimeout(RPI_PING_TIMEOUT);
        RCKid::i2cNumTxBytes_ = RCKid::TX_START;
    // receiving finished, inform main loop we have message waiting
    } else if ((status & I2C_STOP_MASK) == I2C_STOP_RX) {
        TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
        RCKid::flags_.i2cReady = true;
    } else {
        // error - a state we do not know how to handle
    }
    LEAVE_IRQ;
}

#if (defined TEST_CLOCK) 

void setup() {
    // initialize the master clock, allow its output on pin PB5
    CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKOUT_bm;
    CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLB = CLKCTRL_PEN_bm; 
    // enable 8kHz timer identical to that of the audio recording and output it somewhere
}

void loop() {

}


#else 
void setup() {
    RCKid::initialize();
}

void loop() {
    RCKid::loop();
}

#endif


#ifdef OLD

/** RCKid AVR Controller
 
    The AVR is connected diretly to the battery/usb-c power line and manages the power and many peripherals. To the RPI, AVR presents itself as an I2C slave and 2 dedicated pins are used to signal interrupts from AVR to RPI and to signal safe shutdown from RPI to AVR. The AVR is intended to run an I2C bootloader that can be used to update the AVR's firmware when necessary.

                   -- VDD             GND --
             VBATT -- (00) PA4   PA3 (16) -- VIB_EN
          BTN_HOME -- (01) PA5   PA2 (15) -- SCL (I2C)
            BTNS_2 -- (02) PA6   PA1 (14) -- SDA (I2C)
            BTNS_1 -- (03) PA7   PA0 (17) -- (reserved for UPDI)
           MIC_OUT -- (04) PB5   PC3 (13) -- JOY_H
            CHARGE -- (05) PB4   PC2 (12) -- JOY_V
            RGB_EN -- (06) PB3   PC1 (11) -- RPI_POWEROFF
            RPI_EN -- (07) PB2   PC0 (10) -- BACKLIGHT
               RGB -- (08) PB1   PB0 (09) -- AVR_IRQ

    # Power Management

    The chip monitors the current battery voltage (VBATT), temperature (via ADC0), VCC (via internal intref on ADC1) and charging status via the CHARGE pin that is pulled low when charging and HIGH when charge done, if the USB-DC voltage is present, which is only true when VCC is greater than 4.3

    



    Implementation-wise the code is organized in a single all-static class to spare ourselves the pain of forward declarations. 



 */
class RCKid {
public:

    /** Power monitoring uses the analog VBATT and CHARGE pins. CHARGE pin is only useful when DC voltage is applied (VCC > 4.3), when low, the battery is charging, when charge is complete the pin goes to high. 
     */
    static constexpr gpio::Pin VBATT = 0; // PA4, ADC0 channel 4, or ADC1 channel 0
    static constexpr gpio::Pin CHARGE = 5; // PB4, ADC0 channel 9

    /** RPI Management & Communication 
     
        The RPI_EN is connected to an external pull-down resistor so that without an explicit interference from the avr, the RPI is powered on. This is useful for allowing the RPI to program the AVR via I2C. To turn RPI off, the pin must be driven high. Almost all other peripherals derive their power from the RPI_EN as well, including the radio and backlight. The RGB LED is one notable exception. 

        The I2C is used to communicate with the RPI. The RPI is the sole master of the I2C bus. The AVR_IRQ should be left floating and pulled low to signal an event from the AVR. The pin is pulled up by the RPI and should never be held high by the AVR, since the AVR is on larger voltage, the RPI might be damaged in such case. 

        The RPI_POWEROFF pin is used by the RPI to tell it's save to shutdown. 

    */
    static constexpr gpio::Pin RPI_EN = 7;
    static constexpr gpio::Pin AVR_IRQ = 9; // never pull high!!
    static constexpr gpio::Pin RPI_POWEROFF = 11; // never pull high!!
    static constexpr gpio::Pin I2C_SDA = 14; // PA2, (alt) never pull high!!
    static constexpr gpio::Pin I2C_SCL = 15; // PA1, (alt) never pull high!!

    /** Display backlight is only working when RPI_EN is on and is connected to a PWM pin so that its intensity can vary. Uses the alternate position for TCB0.
     */
    static constexpr gpio::Pin BACKLIGHT = 10; // PC0, TCB0 (alt)

    /** Rumbler can be turned both on and off, or being TCB1, the pin can be used to output PWM. Since the rumbler motor is connected to RPI's 3V3 it only works when RPI is enabled. 
     */
    static constexpr gpio::Pin VIB_EN = 16; // PA3, TCB1

    /** The RGB led can be turned on and off independently of the RPI so that it can be used to signal battery conditions and function as a torch without the need to power up the RPI itself. 
     */

    static constexpr gpio::Pin RGB_EN = 6;
    static constexpr gpio::Pin RGB = 8;

    /** Input Controls 
     
        The BTN_HOME pin is pulled up and connected to the home button. It is wired to a dedicated pin so that it can be used to wake up the AVR from deep sleep to power on the device.  

        The BTNS_1 and BTNS_2 pins are connected to ADC1 and they are responsible for reading the dpad and Select & Start buttons. Each button has three of the buttons connected to it a resistor ladder so that multiple presses can be determined based on the value read. BTNS_1 handles left, top and bottom dpad buttons, while BTNS2 handles right dpad and select and start buttons. The pins are pulled up externally and work even without RPI_EN. 

        Finally, the JOY_H and JOY_V are used to read the thumbstick position. Since the thumbstick is powered by the 3.3V from the RPI (RPI is responsible for its button and we might save some power by the resistors being not powered in sleep mode) its value must be adjusted for the changing VCC on AVR and the thumbstick only works if RPI is powered on.  
    */
    static constexpr gpio::Pin BTN_HOME = 1;
    static constexpr gpio::Pin BTNS_1 = 3; // PA7, ADC1 channel 3
    static constexpr gpio::Pin BTNS_2 = 2; // PA6, ADC1 channel 2
    static constexpr gpio::Pin JOY_H = 13; // PC3, ADC1 channel 9
    static constexpr gpio::Pin JOY_V = 12; // PC2, ADC1 channel 8

    /** The microphone is amplified and connected to the MIC_OUT pin. When enabled, the recorder takes full control of ADC0 and samples the output as fast as possible for best resolution. The sampled data is then sent to the RPI via I2C.
     */  
    static constexpr gpio::Pin MIC_OUT = 4; // PB5, ADC0 channel 8

    /** Power modes.
     
        In order 
     */
    enum class Mode {
        PowerOff,
        Booting,
        PowerOn,
        PowerOnCancelled,
        PoweringDown,
        Torch, 
    }; // Mode





    /** Initializes the chip. 
        
     */
    static void initialize() {
        cli();
        // set CLK_PER prescaler to 2, i.e. 10Mhz, which is the maximum the chip supports at voltages as low as 3.3V
        CCP = CCP_IOREG_gc;
        CLKCTRL.MCLKCTRLB = CLKCTRL_PEN_bm; 
        // rpi
        gpio::input(RPI_EN);
        gpio::input(AVR_IRQ);
        gpio::input(RPI_POWEROFF);
        // rumbler
        gpio::input(VIB_EN);
        // rgb led
        gpio::input(RGB_EN);
        gpio::input(RGB);
        // inputs
        gpio::inputPullup(BTN_HOME);
        // initialize the pins connected to ADCs, disable buffers, disable pullup, etc. 
        // initialize the various subsystems
        initializeClocks();
        initializeI2C();



        sei();

    }

    /** Initializes the various clocks used by the AVR. 

        The RTC fires with 1 second period so that we can keep track of real time. TCB0 is used for he 200Hz times at which the input controls are summed up and RPI is notified about state changes. TCB1 is used for the 8kHz signal used for audio recording and TCA is used for the PWM outputs to the backlight and rumbler.  
     */
    static void initializeClocks() {
        // initialize the RTC that fires every second for a semi-accurate real time clock keeping on the AVR
        RTC.CLKSEL = RTC_CLKSEL_INT1K_gc; // select internal oscillator divided by 32
        RTC.PITINTCTRL |= RTC_PI_bm; // enable the interrupt
        RTC.PITCTRLA = RTC_PERIOD_CYC1024_gc + RTC_PITEN_bm;
        // initialize the 200Hz timer for controls (5ms interval). Assuming CLK_PER of 10Mhz, prescale to 5MHz
        TCB0.CTRLB = TCB_CNTMODE_INT_gc;
        TCB0.CCMP = 25000; // for 200Hz
        TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc; // | TCB_ENABLE_bm;
        // initialize the audio timer 
        TCB1.CTRLB = TCB_CNTMODE_INT_gc;
        TCB1.CCMP = 625; // for 8kHz
        TCB1.INTCTRL = TCB_CAPT_bm;
        TCB1.CTRLA = TCB_CLKSEL_CLKDIV2_gc; // | TCB_ENABLE_bm;
        // initialize TCA which we use for the backlight and rumbler PWM output
        // TODO
    }

    static inline void adc0ready() {

    }

    /** ADC1 is used to read the input buttons, joystick measurements and VCC. It cycles through the */
    static inline void adc1ready() {

    }


    static inline volatile struct {
        bool secondTick : 1;
    } flags;



}; // RCKid

/** The RTC one second interval tick ISR. 
 */
ISR(RTC_PIT_vect) {
    RTC.PITINTFLAGS = RTC_PI_bm; // clear the interrupt
    //state.time.secondTick();
    //flags.secondTick = true;
    RCKid::flags.secondTick = true;
}



#define TEST0

#if (defined TEST0)
/** Test 0 is used to determine the clock speed of the AVR and how to set it up properly. It is not to be used when AVR is used inside RCKid but is intended for standalone measurements. 
*/
void setup() {
    // initialize the master clock, allow its output on pin PB5
    CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKOUT_bm;
    CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLB = CLKCTRL_PEN_bm; 
    // enable 8kHz timer identical to that of the audio recording and output it somewhere
}

void loop() {

}

#elif (defined TEST1)
/** A simple test that checks the various connections to the AVR without the need to communicate with the RPI that should be run before the RPI is soldered as the last piece to avoid soldering a rather rare RPi Zero to a defunct board. */
void setup() {

}

void loop() {

}

#else
/** The main app. Delegates all processing to RCKid. 
 */
void setup() {
    RCKid::initialize();

}

void loop() {

}

#endif


/**
    Powering on 

    The AVR is in sleep mode, registering changes on the L and R volume buttons and waking up every second via the RTC timer. When either button is pressed, AVR switches to standby mode, enables the tick timer at 10ms and attempts to determine what 
*/

/** Enables or disables power for the rpi, audioo and most other sensors including buttons, accelermeter and photoresistor. When left floating (default), the power is on so that the rpi can be used to program the AVR without loss of power. To turn the rpi off, the pin must be driven high. 
 
    Connected to a 100k pull-down resistor this means that when rpi is turned off, we need 0.05mA. 
 */

/** Communication with the rpi is done via I2C and an IRQ pin. The rpi is the sole master of the i2c bus. When avr wants to signal rpi a need to communicate, it drives the AVR_IRQ pin low. Otherwise the pin should be left floating as it is pulled up by the rpi. 

TODO can this be used to check rpi is off?  
 */

/** The backlight and vibration motor can be PWM signals using the TCB timer. 
 */
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

/** Determines the state of the battery charger. High means the battery charging has finished. Low means the battery is being charged. There is also a high impedance state when the charger is in shutdown mode, or a battery is not present which we read as analog value of VCC/2. 
 */
static constexpr gpio::Pin CHARGING = 15; // ADC0(2)


/** The entire status of the AVR as a continuous area of memory so that when we rpi reads the I2C all this information can be returned. 
 */
comms::FullState state;

/** Audio buffer
 */
uint8_t i2cBuffer[comms::I2C_PACKET_SIZE * 2];


volatile struct {
    /** Determines that the AVR irq is either requested, or should be requested at the next tick. 
     */
    bool requestIrq : 1;
    bool i2cReady : 1;
    bool secondTick : 1;
    bool sleep : 1;
} flags;



namespace audio {
    void startRecording();
    void stopRecording();
}

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
        //_PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_PEN_bm);
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
    uint8_t i2cBuffer_[comms::I2C_PACKET_SIZE];


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

    /** Requests the IRQ pin to be pulled down on the next tick. 
     
        IRQ requests are ignored while audio recording is in progress as there is no way to distingush between state change IRQ and packet ready IRQ signal. 
     */
    void requestIrq() {
        if (! state.status.recording())
            flags.requestIrq = true;
    }

    void tick() {
        // force IRQ down if requested and the pin is not output pin yet
        if (flags.requestIrq == true) {
           flags.requestIrq = false;
            if ((PORTA.DIR & (1 << 4)) == 0) {
                gpio::output(AVR_IRQ);
                gpio::low(AVR_IRQ);
            }
        }
    }

    /** Sets the address from which the next I2C read will be. 
     */
    void setNextReadAddress(uint8_t * addr) {
        cli();
        i2cReadStart_ = addr;
        sei();
    }


    void processCommand() {
        msg::Message const & m = msg::Message::fromBuffer(i2cBuffer_);
        switch (i2cBuffer_[0]) {
            // resets the chip
            case msg::AvrReset::Id: {
                _PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRE_bm);
            }
            // identical to the info of bootloader, next reading of data will return the chip info in the same format as the bootloader. 
            // NOTE it is not allowed to issue this message while recording audio
            case msg::Info::Id: {
                uint8_t * buffer = i2cBuffer_ + 1;
                buffer[0] = SIGROW.DEVICEID0;
                buffer[1] = SIGROW.DEVICEID1;
                buffer[2] = SIGROW.DEVICEID2;
                buffer[3] = 1; // app
                for (uint8_t i = 0; i < 10; ++i)
                    buffer[4 + i] = ((uint8_t*)(&FUSE))[i];
                buffer[15] = CLKCTRL.MCLKCTRLA;
                buffer[16] = CLKCTRL.MCLKCTRLB;
                buffer[17] = CLKCTRL.MCLKLOCK;
                buffer[18] = CLKCTRL.MCLKSTATUS;
                buffer[19] = MAPPED_PROGMEM_PAGE_SIZE >> 8;
                buffer[20] = MAPPED_PROGMEM_PAGE_SIZE & 0xff;
                setNextReadAddress(i2cBuffer_ + 1);
                break;
            }
            case msg::ClearPowerOnFlag::Id: {
                state.status.setAvrPowerOn(false);
                break;
            }
            case msg::StartAudioRecording::Id: {
                audio::startRecording();
                break;
            }
            case msg::StopAudioRecording::Id: {
                audio::stopRecording();
                break;
            }
            case msg::SetBrightness::Id: {
                state.estatus.setBrightness(m.as<msg::SetBrightness>().value);
                analogWrite(BACKLIGHT, state.estatus.brightness());
                break;
            }
            case msg::PowerOff::Id: {
                // TODO
            }
        }

        // clear the received bytes buffer and the i2c command ready flag
        i2cBytesRecvd_ = 0;
        cli();
        flags.i2cReady = false;
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
        // sending data to accepting master is on our fastpath and is checked first, if there is more state to send, send next byte, otherwise go to transaction completed mode. 
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
            // If read start was one byte into the buffer, which we use for the chip info message buffer shared with bootloader, switch immediately to reading state in the next message
            if (i2cReadStart_ == i2cBuffer_ + 1)
                i2cReadStart_ = reinterpret_cast<uint8_t*>(& state);
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
    static constexpr uint8_t MUXPOS_CHARGING = ADC_MUXPOS_AIN2_gc;

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
        static_assert(CHARGING == 15); // AIN2 - ADC0, PA2
        PORTA.PIN2CTRL &= ~PORT_ISC_gm;
        PORTA.PIN2CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTA.PIN2CTRL &= ~PORT_PULLUPEN_bm;
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
        /*
        if (state.estatus.irqMic() && micMax_ >= state.estatus.micThreshold()) 
            if (state.status.setMicLoud())
                rpi::requestIrq();
            */
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
                state.estatus.setVcc(value);
                if (value <= CRITICAL_BATTERY_THRESHOLD) {
                    // TODO what to do? 
                } else if (value <= LOW_BATTERY_THRESHOLD) {
                    if (state.status.setLowBattery())
                        rpi::requestIrq();
                } else if (value >= VUSB_BATTERY_THRESHOLD) {
                    if (state.status.setVUsb())
                      rpi::requestIrq();
                }   
                ADC0.MUXPOS = MUXPOS_VBATT;
                break;
            /* Using the VCC value, calculates the VBATT in 10mV increments, i.e. [V*100]. Expected values are 0..430.
             */
            case MUXPOS_VBATT:
                //value = state.estatus.vcc() * value / 1024;
                value = state.estatus.vcc() * (value >> 3) / 128;
                state.estatus.setVBatt(value);
                ADC0.MUXPOS = MUXPOS_CHARGING;
                break;
            case MUXPOS_CHARGING:
                // only report charging when vcc is greter than the VUSB threshold (meaning a charger is actually connected) 
                if (state.estatus.vcc() >= VUSB_BATTERY_THRESHOLD) {
                    if (value < 128) {
                        // logical 0, charging, vbatt reading tells the progress
                        if (state.status.setCharging(true)) 
                            rpi::requestIrq();
                    } else if (value > 896) {
                        // logical 1, battery charging finished
                        if (state.status.setCharging(false))
                            rpi::requestIrq();
                    } else {
                        // hi-Z state, battery not present, or charger in shutdown mode, we report it as not charging because we do not care that much
                        if (state.status.setCharging(false))
                            rpi::requestIrq();
                    }
                } else {
                    if (state.status.setCharging(false))
                        rpi::requestIrq();
                }
                ADC0.MUXPOS = MUXPOS_PHOTORES;
                break;
            /* Calculates the photoresistor value. Expected range is 0..255 which should correspond to 0..VCC voltage measured. 
             */
            case MUXPOS_PHOTORES:
                if (state.status.setPhotores((value >> 2) & 0xff) && state.estatus.irqPhotores())
                    rpi::requestIrq();
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
                state.estatus.setTemp(temp);
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

    uint8_t x = 0;

    void startRecording() {
        state.status.setRecording(true);
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
        TCB1.CCMP = 400; // for 8kHz
        TCB1.INTCTRL = TCB_CAPT_bm;
        TCB1.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
        recAcc_ = 0;
        recAccSize_ = 0;
        bufferIndex_ = 0;
        x = 0;
    }

    /** Stops the audio recording by disabling the 8kHz timer and returning ADC0 to the single conversion mode used to sample multiple perihperals and voltages. 
     */
    void stopRecording() {
        ADC0.CTRLA = 0;
        ADC0.INTCTRL = 0;
        TCB1.CTRLA = 0;
        cli(); // make sure we are not sending the last mic packet
        state.status.setRecording(false);
        rpi::i2cReadStart_ = reinterpret_cast<uint8_t*>(& state);
        sei();
        adc0::start();
    }

    ISR(TCB1_INT_vect) {
        TCB1.INTFLAGS = TCB_CAPT_bm;
        if (recAccSize_ > 0) 
            recAcc_ /= recAccSize_;
        // store the value in buffer
        i2cBuffer[bufferIndex_] = x++; // (recAcc_ & 0xff);
        bufferIndex_ = (bufferIndex_ + 1) % (comms::I2C_PACKET_SIZE * 2);
        if (bufferIndex_ % comms::I2C_PACKET_SIZE == 0) {
            rpi::setNextReadAddress(bufferIndex_ != 0 ? i2cBuffer : i2cBuffer + comms::I2C_PACKET_SIZE);
            // drive the IRQ pin low immediatety
            gpio::output(AVR_IRQ);
            gpio::low(AVR_IRQ);
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

    uint8_t debounceCounter_[2] = {0,0};

    // forward declaration for the ISR
    void volumeLeft();
    void volumeRight();
    void joystickButton();

    void initialize() {
        // start pullups on the L and R volume buttons
        gpio::inputPullup(BTN_LVOL);
        gpio::inputPullup(BTN_RVOL);
        // invert the button ports so that we get 1 for button pressed, 0 for released
        static_assert(BTN_LVOL == 14);
        PORTA.PIN1CTRL |= PORT_INVEN_bm;
        static_assert(BTN_RVOL == 6);
        PORTB.PIN3CTRL |= PORT_INVEN_bm;
        // attach button interrupts on change
        attachInterrupt(digitalPinToInterrupt(BTN_LVOL), volumeLeft, CHANGE);
        attachInterrupt(digitalPinToInterrupt(BTN_RVOL), volumeRight, CHANGE);
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
    }

    void joystickStop() {
        // simply disable ADC1
        ADC1.CTRLA = 0;
    }

    bool joystickReady() {
        return (ADC1.INTFLAGS & ADC_RESRDY_bm);
    }

    void processJoystick() {
        uint8_t value = (ADC1.RES / 64) >> 2;
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

    void volumeLeft() {
        if (debounceCounter_[0] == 0) {
            debounceCounter_[0] = DEBOUNCE_TICKS;
            if (state.status.setBtnVolumeLeft(gpio::read(BTN_LVOL)))
                rpi::requestIrq();
        }
    }

    void volumeRight() {
        if (debounceCounter_[1] == 0) {
            debounceCounter_[1] = DEBOUNCE_TICKS;
            if (state.status.setBtnVolumeRight(gpio::read(BTN_RVOL)))
                rpi::requestIrq();
        }
    }
    
    /** Each tick decrements the debounce timers and if they reach zero, the button status if updated and if it changes, irq is set (in this case press or release was shorted than the debounce interval). During the tick we also update the X and Y joystick readings (averaged over the measurements that happened during the tick for less jitter). 
     */
    void tick() {
        if (debounceCounter_[0] > 0)
            if (--debounceCounter_[0] == 0)
                if (state.status.setBtnVolumeLeft(gpio::read(BTN_LVOL)))
                    rpi::requestIrq();
        if (debounceCounter_[1] > 0)
            if (--debounceCounter_[1] == 0)
                if (state.status.setBtnVolumeRight(gpio::read(BTN_RVOL)))
                    rpi::requestIrq();
        if (accHSize_ != 0) {
            accH_ = accH_ / accHSize_;
            if (state.status.setJoyX(accH_))
                rpi::requestIrq();
        }
        if (accVSize_ != 0) {
            accV_ = accV_ / accVSize_;
            if (state.status.setJoyY(accV_))
                rpi::requestIrq();
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


#endif