#pragma once

#include "platform/color.h"

#include "widget.h"
#include "window.h"

/** The torchlight control widget. 

    Supports changing the light color either from presets, or by joystick, or the accelerometer. Also turns the light on or off.  
*/
class Torchlight : public Widget {
public:    
    Torchlight(Window * window): Widget{window} {}

protected:

    void draw() override { }

    void btnA(bool state) override {
        if (state)
            window()->rckid()->rgbColor(platform::Color::Blue().withBrightness(16));
    }

    void btnX(bool state) override {
        if (state)
            window()->rckid()->rgbColor(platform::Color::Red().withBrightness(16));
    }

    void btnY(bool state) override {
        if (state)
            window()->rckid()->rgbColor(platform::Color::Green().withBrightness(16));
    }

    void onNavigationPop() override {
        window()->rckid()->rgbOff();
    }

    void onNavigationPush() override {
        window()->rckid()->rgbOn();
    }
}; // Torchlight