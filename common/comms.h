#pragma once

#include "platform/platform.h"
#include "utils/time.h"

namespace comms {

    static constexpr uint8_t AVR_I2C_ADDRESS = 0x43;

    static constexpr size_t I2C_BUFFER_SIZE = 32;

    /** The state AVR sends to RP2040. 
     
        This consists of the state of the buttons, 
    */
    class Status {
    public:
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

        DateTime const & time() const { return time_; }
        DateTime & time() { return time_; }

        bool powerOn() const { return status_ & POWER_ON; }
        void setPowerOn(bool value = true) { setOrClear(status_, POWER_ON, value); }

    private:
        static constexpr uint8_t BTN_LVOL = 1 << 0;
        static constexpr uint8_t BTN_RVOL = 1 << 1;
        static constexpr uint8_t BTN_JOY = 1 << 2;
        //static constexpr uint8_t = 1 << 3;
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

        /** Battery voltage. 
         */
        uint8_t batt_;

        /** Temperature.
         */
        uint8_t temp_;

        DateTime time_;

    } __attribute__((packed)); // State

    static_assert(sizeof(Status) == 10);

}