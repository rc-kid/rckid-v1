#pragma once

#include "platform/platform.h"
#include "utils/time.h"

namespace comms {

    static constexpr uint8_t AVR_I2C_ADDRESS = 0x43;

    static constexpr size_t I2C_BUFFER_SIZE = 32;

    /** Basic AVR Status. 
     
     */
    class Status {
    public:

        bool avrPowerOn() const { return status_ & POWER_ON; }
        bool charging() const { return status_ & CHARGING; }
        bool vusb() const { return status_ & VUSB; }
        bool lowBattery() const { return status_ & LOW_BATT; }

        void setAvrPowerOn(bool value = true) { setOrClear(status_, POWER_ON, value); }
        void setCharging(bool value = true) { setOrClear(status_, CHARGING, value); }
        void setVUsb(bool value = true) { setOrClear(status_, VUSB, value); }
        void setLowBattery(bool value = true) { setOrClear(status_, LOW_BATT, value); }

        bool micLoud() const { return status_ & MIC_LOUD; }
        bool setMicLoud(bool value = true) { return checkSetOrClear(status_, MIC_LOUD, value); }

        bool btnLeftVolume() const { return status_ & BTN_LVOL; }
        bool btnRightVolume() const { return status_ & BTN_RVOL; }
        bool btnJoystick() const { return status_ & BTN_JOY; }
        uint8_t joyX() const { return joyX_; }
        uint8_t joyY() const { return joyY_; }

        bool setBtnLeftVolume(bool value) { return checkSetOrClear(status_, BTN_LVOL, value); }
        bool setBtnRightVolume(bool value) { return checkSetOrClear(status_, BTN_RVOL, value); }
        bool setBtnJoystick(bool value) { return checkSetOrClear(status_, BTN_JOY, value); }
        bool setJoyX(uint8_t value) { 
            if (joyX_ != value) {
                joyX_ = value;
                return true;
            } else {
                return false;
            } 
        }
        bool setJoyY(uint8_t value) { 
            if (joyY_ != value) {
                joyY_ = value;
                return true;
            } else {
                return false;
            }
        }

        uint8_t photores() const { return photores_; }
        bool setPhotores(uint8_t value) {
            if (photores_ != value) {
                photores_ = value;
                return true;
            } else {
                return false;
            }
        }


    private:
        static constexpr uint8_t BTN_LVOL = 1 << 0;
        static constexpr uint8_t BTN_RVOL = 1 << 1;
        static constexpr uint8_t BTN_JOY = 1 << 2;
        static constexpr uint8_t MIC_LOUD = 1 << 3;
        static constexpr uint8_t POWER_ON = 1 << 4;
        static constexpr uint8_t CHARGING = 1 << 5;
        static constexpr uint8_t VUSB = 1 << 6;
        static constexpr uint8_t LOW_BATT = 1 << 7;

        uint8_t status_ = 0;

        /** Joystick X and Y coordinates. 
         */
        uint8_t joyX_;
        uint8_t joyY_;

        /** Photoresistor value (unitless)
         */
        uint8_t photores_;
    } __attribute__((packed)); // Status

    static_assert(sizeof(Status) == 4);

    class ExtendedStatus {
    public:

        /** Returns the vcc voltage (battery or USB when attached)
         

            500 = 5V or more
            250 = 2.5V
            0 = below 2.46V
            
        */
       /*
        uint16_t vcc() const {
            return (vcc_ == 0) ? 0 : (vcc_ + 245);
        } */

/*
        void setVcc(uint16_t vx100) {
            if (vx100 < 250)
                vcc_ = 0;
            else if (vx100 >= 500)
                vcc_ = 255;
            else 
                vcc_ = (vx100 - 249) & 0xff;
        } */

        /** Returns the temperature. 
         
            -200 = -20C or less
            -195 = -19.5
            0 = 0
            1080 = 108C or more
        */

        int16_t temp() const {
            return -200 + (temp_ * 5);
        }

        void setTemp(int32_t tempx10) {
            if (tempx10 <= -200)
                temp_ = 0;
            else if (tempx10 >= 1080)
                temp_ = 255;
            else 
                temp_ = (tempx10 + 200) / 5;
        }

        uint8_t micThreshold() const { return micThreshold_; }
        void setMicThreshold(uint8_t value) { micThreshold_ = value; }

        uint8_t brightness() const { return brightness_; }
        void setBrightness(uint8_t value) { brightness_ = value; }

        // 4456

        bool irqPhotores() const { return settings_ & IRQ_PHOTORES; }
        bool irqMic() const { return settings_ & IRQ_MIC; }

    private:
        static constexpr uint8_t IRQ_PHOTORES = 1 << 0;
        static constexpr uint8_t IRQ_MIC = 1 << 1;

        uint8_t settings_;

        uint8_t vcc_ = 0;
        uint8_t batt_ = 0;
        uint8_t temp_ = 0;
        uint8_t micThreshold_ = 255;
        uint8_t brightness_ = 64;

    } __attribute__((packed)); // ExtendedStatus
}

#define MESSAGE(NAME, ...) \
    class NAME : public msg::MessageHelper<NAME> { \
    public: \
        static uint8_t constexpr Id = __COUNTER__ - COUNTER_OFFSET; \
        __VA_ARGS__ \
    } __attribute__((packed));

namespace msg {
    class Message {
    public:
        uint8_t const id;

        template<typename T> T const & as() const {
            return * reinterpret_cast<T const *>(this);
        }

        static Message const & fromBuffer(uint8_t const * buffer) {
            return * reinterpret_cast<Message const *>(buffer);
        }

    protected:
        Message(uint8_t id): id{id} {}
        static int constexpr COUNTER_OFFSET = __COUNTER__ + 1;
    };

    template<typename T> class MessageHelper : public Message {
    protected:
        MessageHelper():Message{T::Id} {}
    }; // MessageHelper<T>


    /** Clears the power on flag in the AVR status. 
     */
    MESSAGE(ClearPowerOnFlag)

    /** Sets the brightness of the TFT screen. 
     */
    MESSAGE(SetBrightness,
        uint8_t value;
        SetBrightness(uint8_t value): value{value} {}
    )


} // namespace msg

#undef MESSAGE

