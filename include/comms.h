#pragma once

#include "platform/platform.h"
#include "utils/time.h"

static constexpr uint8_t AVR_I2C_ADDRESS = 0x43;

static constexpr size_t I2C_BUFFER_SIZE = 32;

static constexpr uint8_t BTN_PWR = 0;
static constexpr uint8_t BTN_A = 1;
static constexpr uint8_t BTN_B = 2;
static constexpr uint8_t BTN_C = 3;
static constexpr uint8_t BTN_D = 4;
static constexpr uint8_t BTN_L = 5;
static constexpr uint8_t BTN_R = 6;

/** The state AVR sends to RP2040. 
 
    This consists of the state of the buttons, 
 */
class State {
public:
    bool btnPwr() const { return buttons_ & (1 << BTN_PWR); }
    bool btnA() const { return buttons_ & (1 << BTN_A); }
    bool btnB() const { return buttons_ & (1 << BTN_B); }
    bool btnC() const { return buttons_ & (1 << BTN_C); }
    bool btnD() const { return buttons_ & (1 << BTN_D); }
    bool btnLeft() const { return buttons_ & (1 << BTN_L); }
    bool btnRight() const { return buttons_ & (1 << BTN_R); }
    uint8_t joyX() const { return joyX_; }
    uint8_t joyY() const { return joyY_; }

    void setBtnPwr(bool value) { setOrClear(buttons_, 1 << BTN_PWR, value); }
    void setBtnA(bool value) { setOrClear(buttons_, 1 << BTN_A, value); }
    void setBtnB(bool value) { setOrClear(buttons_, 1 << BTN_B, value); }
    void setBtnC(bool value) { setOrClear(buttons_, 1 << BTN_C, value); }
    void setBtnD(bool value) { setOrClear(buttons_, 1 << BTN_D, value); }
    void setBtnLeft(bool value) { setOrClear(buttons_, 1 << BTN_L, value); }
    void setBtnRight(bool value) { setOrClear(buttons_, 1 << BTN_R, value); }
    void setJoyX(uint8_t value) { joyX_ = value; }
    void setJoyY(uint8_t value) { joyY_ = value; }

    bool button(uint8_t index) const {
        return buttons_ & static_cast<uint8_t>(1 << index);
    }

    void setButton(uint8_t index, bool value) {
        setOrClear(buttons_, 1 << index, value);
    }

    /** Returns the vcc voltage (battery or USB when attached)
     

        500 = 5V or more
        250 = 2.5V
        0 = below 2.46V
        
     */
    uint16_t vcc() const {
        return (vcc_ == 0) ? 0 : (vcc_ + 245);
    }

    void setVcc(uint16_t vx100) {
        if (vx100 < 250)
            vcc_ = 0;
        else if (vx100 >= 500)
            vcc_ = 255;
        else 
            vcc_ = (vx100 - 249) & 0xff;
    }

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

private:

    uint8_t buttons_;
    uint8_t joyX_;
    uint8_t joyY_;

    uint8_t vcc_;
    uint8_t batt_;
    uint8_t temp_;

    DateTime time_;

} __attribute__((packed)); // State

static_assert(sizeof(State) == 10);