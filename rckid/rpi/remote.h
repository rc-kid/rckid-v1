#pragma once

#include "widget.h"
#include "window.h"

#include "remote/lego_remote.h"

class Remote : public Widget {
public:
    Remote(Window * window): Widget{window} {}
protected:
    void draw() override {
    }


    void onFocus() override {
        window()->rckid()->nrfInitialize("BBBBB", "AAAAA", 86);
        window()->rckid()->nrfEnableReceiver();
    }

    void onBlur() override {
        window()->rckid()->nrfStandby();
    }

    void btnA(bool state) override {
        using namespace remote;
        if (state) {
            msg_.ctrl.mode = channel::Motor::Mode::Brake;
            msg_.ctrl.speed = 0;
            window()->rckid()->nrfTransmit(reinterpret_cast<uint8_t*>(&msg_), sizeof(msg_));
        } 
    }

    void btnX(bool state) override {
        using namespace remote;
        if (state) {
            msg_.ctrl.mode = channel::Motor::Mode::CW;
            msg_.ctrl.speed = speed_;
            speed_ += 1;
            window()->rckid()->nrfTransmit(reinterpret_cast<uint8_t*>(&msg_), sizeof(msg_));
        } 
    }

private:
    //LegoRemote::Feedback feedback_;
    uint8_t speed_ = 0;

    struct {
        uint8_t channel = remote::LegoRemote::CHANNEL_ML;
        remote::channel::Motor::Control ctrl;
        uint8_t eof = 0;
    } __attribute__((packed)) msg_;

    static_assert(sizeof(msg_) == 4);

}; // Remote