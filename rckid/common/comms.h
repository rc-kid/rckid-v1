#pragma once

#include "platform/platform.h"
#include "utils/time.h"

#include "config.h"

namespace comms {

    /** Power-related Modes the RCKid can be in. 

        When the BTN_HOME button is pressed, we enter the `PoweringUp` mode, in which the power to RPI is enabled and the RPI starts to boot. The AVR also times the length of the BTN_HOME press. If the press duration goes over the POWER_ON_PRESS_THRESHOLD_TICKS, the rumbler is signalled. If the btn button is pressed for a shorter period, mode changes to `PoweringDown`.

        Eventually the RPi will contact the AVR, changing the mode from `PoweringOn` to `On`.  If the mode is already `PoweringDown`, the RPi should power itself off immediately, otherwise it starts working normally and AVR enabled the backlight.   

        When in the `PoweringDown` mode, the AVR waits for the `RPI_POWEROFF` signal to go up, at which point it turns the power to Rpi off and changes mode to sleep.  

        After power on, the default mode is `On`.  
     */
    enum class Mode : uint8_t {
        On = 0,  // also dubs as initial recording batch index (we only record in On mode)
        Sleep, 
        WakeUp,
        PowerUp,
        PowerDown,
    }; // comms::Mode


    /** Error codes. 
     
        The extended state also contains an information about the error state the RCKid is in. While not all states are really errors (such as initial power on), they should all be inspected, reported and logged by the RPi appropriately. Depending on the error state. For some states, the RGB LED also illustrates what is going on. 
    */
    enum class ErrorCode : uint8_t {
        /// no error condition present
        NoError, 
        /// initial power on (not an error condition)
        InitialPowerOn,
        /// AVR watchdog timed out
        WatchdogTimeout,
        /// RPi failed to boot in time 
        RPiBootTimeout,
        /// RPi failed to request state often enough
        RPiPingTimeout, 
        /// RPi failed to power down in the specified timeout
        RPiPowerDownTimeout,
    }; 

    /** The AVR status. 

        Every I2C communication with the AVR always returns the status as the first byte. 
     */
    class Status {
    public:

        Mode mode() const { return recording() ? Mode::On : static_cast<Mode>(status_ & MODE); }

        void setMode(Mode mode) {
            status_ = status_ & ~MODE;
            status_ |= static_cast<uint8_t>(mode);
        }

        bool recording() const { return status_ & RECORDING; }

        bool setRecording(bool value) { 
            if (value == charging())
                return false;
            value ? (status_ |= RECORDING) : (status_ &= ~RECORDING);
            return true;
        }

        bool usb() const { return status_ & USB_DC; }

        bool setUsb(bool value) {
            if (value == usb())
                return false;
            value ? (status_ |= USB_DC) : (status_ &= ~USB_DC);
            return true;
        }

        bool charging() const { return status_ & CHARGING; }

        bool setCharging(bool value) { 
            if (value == charging())
                return false;
            value ? (status_ |= CHARGING) : (status_ &= ~CHARGING);
            return true;
        }

        bool lowBatt() const { return status_ & LOW_BATT; }

        bool setLowBatt(bool value) { 
            if (value == lowBatt())
                return false;
            value ? (status_ |= LOW_BATT) : (status_ &= ~LOW_BATT);
            return true;
        }

        bool alarm() const { return status_ & ALARM; }

        bool setAlarm(bool value) { 
            if (value == alarm())
                return false;
            value ? (status_ |= ALARM) : (status_ &= ~ALARM);
            return true;
        }

        /** In recording mode, returns the index of the next batch that will be returned if the reading continues.
         */
        uint8_t batchIndex() const {
            return status_ & MODE;
        }

        void setBatchIndex(uint8_t index) {
            status_ &= ~MODE;
            status_ |= (index & MODE);
        }

    private:

        static constexpr uint8_t MODE = 7;
        static constexpr uint8_t ALARM = 1 << 3;
        static constexpr uint8_t RECORDING = 1 << 4;
        static constexpr uint8_t USB_DC = 1 << 5;
        static constexpr uint8_t CHARGING = 1 << 6;
        static constexpr uint8_t LOW_BATT = 1 << 7;

        uint8_t status_ = 0;
    }; // comms::Status

    /** Information about the input controls connected to the RPi. 
    */
    class Controls {
    public:

        bool dpadLeft() const { return buttons_ & DPAD_LEFT; }
        bool dpadRight() const { return buttons_ & DPAD_RIGHT; }
        bool dpadTop() const { return buttons_ & DPAD_TOP; }
        bool dpadBottom() const { return buttons_ & DPAD_BOTTOM; }
        bool select() const { return buttons_ & SELECT; }
        bool start() const { return buttons_ & START; }
        bool home() const { return buttons_ & HOME; }

        /** Sets the button values all at once. 
         
            Note that this "hacky" function requires the button's bits to align with the bits reported by the analog button decoder. 
        */
        bool setButtons(uint8_t btns1, uint8_t btns2, bool btnHome) {
            uint8_t btns = (btnHome ? HOME : 0) | (btns1 & 0x7) | ((btns2 & 0x7) << 3);
            if (btns == buttons_)
                return false;
            buttons_ = btns;
            return true;
        }

        uint8_t joyH() const { return joyH_; }

        bool setJoyH(uint8_t value) {
            if (value == joyH_)
                return false;
            joyH_ = value;
            return true;
        }

        uint8_t joyV() const { return joyV_; }

        bool setJoyV(uint8_t value) {
            if (value == joyV_)
                return false;
            joyV_ = value;
            return true;
        }

    private:
        static constexpr uint8_t DPAD_LEFT = 1 << 0;
        static constexpr uint8_t DPAD_RIGHT = 1 << 1;
        static constexpr uint8_t DPAD_TOP = 1 << 2;
        static constexpr uint8_t DPAD_BOTTOM = 1 << 3;
        static constexpr uint8_t SELECT = 1 << 4;
        static constexpr uint8_t START = 1 << 5;
        static constexpr uint8_t HOME = 1 << 6;

        uint8_t buttons_;
        uint8_t joyH_;
        uint8_t joyV_;
    }; // comms::Controls 

    class ExtendedInfo {
    public:

        /** \name VCC
         
            The voltage to the AVR measured in 0.01[V]. Value of 0 means any voltage below 2.46V, value of 500 5V or more, otherwise the number returnes is volatge * 100. 
         */
        //@{
        uint16_t vcc() const { return (vcc_ == 0) ? 0 : (vcc_ + 245); }

        void setVcc(uint16_t vx100) {
            if (vx100 < 250)
                vcc_ = 0;
            else if (vx100 >= 500)
                vcc_ = 255;
            else 
                vcc_ = (vx100 - 245) & 0xff;
        }
        //@}

        /** \name VBATT
         
            Battery voltage. Measured in 0.01[V] in rangle from 1.7V to 4.2V. 420 means 4.2V or more, 0 means 1.7V or less.
         */
        //@{
        uint16_t vbatt() const { return (vbatt_ == 0) ? 0 : vbatt_ + 165; }

        void setVBatt(uint16_t vx100) {
            if (vx100 >= 420)
                vbatt_ = 255;
            else if (vx100 < 170)
                vbatt_ = 0;
            else 
                vbatt_ = (vx100 - 165) & 0xff;
        }
        //@}

        /** \name Temperature 
         
            Returns the temperature as measured by the chip with 0.1[C] intervals. -200 is -20C or less, 1080 is 108[C] or more. 0 is 0C. 
         */
        //@{
        int16_t temp() const { return -200 + (temp_ * 5); }

        void setTemp(int32_t tempx10) {
            if (tempx10 <= -200)
                temp_ = 0;
            else if (tempx10 >= 1080)
                temp_ = 255;
            else 
                temp_ = (tempx10 + 200) / 5;
        }
        //@}

        /** \name LCD Brightness. 
         */
        //@{
        uint8_t brightness() const { return brightness_; }

        void setBrightness(uint8_t value) { brightness_ = value; }
        //@}

    private:
        uint8_t vcc_;
        uint8_t vbatt_;
        uint8_t temp_;
        uint8_t brightness_;
    }; // comms::ExtendedInfo;

    /** State consists  */
    class State {
    public:
       Status status;
       Controls controls;
    }; // comms::State

    static_assert(sizeof(State) == 4);


    class DebugInfo {
    public:
        ErrorCode errorCode() const { return static_cast<ErrorCode>(raw_ & ERROR_CODE); }

        void setErrorCode(ErrorCode code) {
            raw_ &= ~ERROR_CODE;
            raw_ |= static_cast<uint8_t>(code) & ERROR_CODE;
        }

        bool repairMode() const { return raw_ & REPAIR_MODE; }
        void setRepairMode(bool value) { 
            value ? (raw_ |= REPAIR_MODE) : (raw_ &= ~REPAIR_MODE);
        }
    
    private:
        static constexpr uint8_t ERROR_CODE = 15;
        static constexpr uint8_t REPAIR_MODE = 1 << 7;
        // 3 free bits
        uint8_t raw_;

    }; // comms::DebugInfo

    class ExtendedState {
    public:
        Status status;
        Controls controls;
        ExtendedInfo einfo;
        DebugInfo dinfo;
        platform::DateTime time;
        platform::DateTime alarm;
    }; // comms::ExtendedState

    static_assert(sizeof(ExtendedState) <= 32);

} // namespace comms

/** \name Messages
 
    The AVR and RPi communicate with each other via typed messages whose kinds and payloads are defined below. All messages have automatically assigned id numbers. As a side-effect the avr and rpi versions must be identical or the message ids may differ, which would lead to problems. 
 */

#define MESSAGE(NAME, ...) \
    class NAME : public msg::MessageHelper<NAME> { \
    public: \
        static uint8_t constexpr ID = __COUNTER__ - COUNTER_OFFSET; \
        static NAME const & fromBuffer(uint8_t const * buffer) { return * reinterpret_cast<NAME const *>(buffer); } \
        __VA_ARGS__ \
    } __attribute__((packed))

namespace msg {

    class Message {
    public:
        uint8_t const id;

        /*
        template<typename T> T const & fromBuffer(uint8_t const * buffer) {
            return * reinterpret_cast<T const *>(buffer);
        } */

    protected:
        Message(uint8_t id): id{id} {}
        static int constexpr COUNTER_OFFSET = __COUNTER__ + 1;
    };

    template<typename T> class MessageHelper : public Message {
    public:
        MessageHelper():Message{T::ID} {}
    }; // MessageHelper<T>

    /** First message is NOP. This is for compatibility with the bootloader where message 0x00 is reserved. 
     */
    MESSAGE(Nop);
    static_assert(Nop::ID == 0);

    /** Causes the reset of the AVR chip. 
     
        Useful for debugging and programming via the I2C interface. Reset in the AVR mode is the same as in the bootloader. 
     */
    MESSAGE(AvrReset);
    static_assert(AvrReset::ID == 1);

    /** Returns the chip information. This is identical to the bootloader and can be used to get chip information after it has been programmed. 
     */
    MESSAGE(Info);
    static_assert(Info::ID == 2);

    /** Starts the audio recording. 
    */
    MESSAGE(StartAudioRecording);
    /** Stops the audio recording.
     */
    MESSAGE(StopAudioRecording);

    /** Sets the brightness of the TFT screen. 
     */
    MESSAGE(SetBrightness,
        uint8_t value;
        SetBrightness(uint8_t value): value{value} {}
    );

    MESSAGE(SetTime, 
        platform::DateTime value;
        SetTime(platform::DateTime value): value{value} {}
    );

    MESSAGE(SetAlarm, 
        platform::DateTime value;
        SetAlarm(platform::DateTime value): value{value} {}
    );

    MESSAGE(RumblerOk);
    MESSAGE(RumblerFail);
    MESSAGE(Rumbler,
        uint8_t intensity;
        uint16_t duration;
        Rumbler(uint8_t intensity, uint16_t duration): intensity{intensity}, duration{duration} {}
    );

    /** Informs the AVR that the raspberry pi has been powered on and is working. 
     
        Clears the power up mode timeout and moves to the On mode with periodic AVR status check intervals. 
     */
    MESSAGE(PowerOn);

    /** Powers the raspberry pi off and moves the avr into a standby state. 
     
        The poweroff is not immediate since RPI must 
     */
    MESSAGE(PowerDown);

    /** Allows programatically entering the repair mode. 
     */
    MESSAGE(EnterRepairMode);
    /** Programatically deactivates the repair mode.
     */
    MESSAGE(LeaveRepairMode);



} // namespace msg

#undef MESSAGE
