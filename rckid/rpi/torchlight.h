#pragma once

#include "platform/color.h"

#include "widget.h"
#include "window.h"

/** The torchlight control widget. 

    Supports changing the light color either from presets, or by joystick, or the accelerometer. Also turns the light on or off.  


*/
class Torchlight : public Widget {
public:    

protected:

    void draw(Canvas &) override { 
        DrawCircle(160, 90, 60, Color{r_, 0, 0, r_});
        DrawCircle(130, 140, 60, Color{0, g_, 0, g_});
        DrawCircle(190, 140, 60, Color{0, 0, b_, b_});
        DrawCircleLines(160, 90, 60, Color{255, 0, 0, 255});
        DrawCircleLines(130, 140, 60, Color{0, 255, 0, 255});
        DrawCircleLines(190, 140, 60, Color{0, 0, 255, 255});
        cancelRedraw();
    }

    void btnA(bool state) override {
        if (state) {
            b_ += 16;
            updateColor();
        }
    }

    void btnX(bool state) override {
        if (state) {
            r_ += 16;
            updateColor();
        }
    }

    void btnY(bool state) override {
        if (state) {
            g_ += 16;
            updateColor();
        }
    }

    void onNavigationPop() override {
        rckid().rgbOff();
    }

    void onNavigationPush() override {
        rckid().rgbOn();
    }

    void updateColor() {
        requestRedraw();
        rckid().rgbColor(platform::Color::RGB(r_, g_, b_));
    }

    uint8_t r_ = 16;
    uint8_t g_ = 16;
    uint8_t b_ = 16;
}; // Torchlight
