#pragma once

#include <functional>

#include "widget.h"
#include "window.h"
#include "animation.h"


/** A simple widget that displays a wait icon that rotates with the FPS. 
 */
class Wait : public Widget {
public:

    void show(std::function<void(Wait*)> tickHandler) {
        tickHandler_ = tickHandler;
        window().showWidget(this);
    }

    void showDelay(float ms) {
        tWait_.start(ms);
        show([](Wait * w) {
            if (w->tWait_.done())
                window().back();
        });
    }

    void showDelay(float ms, std::function<bool(Wait*)> tickHandler) {
        tWait_.start(ms);
        show([this, tickHandler](Wait * w) {
            if (w->tWait_.done())
                if (tickHandler(this)) 
                    window().back();
                else
                    w->tWait_.start();
        });
    }


protected:

    void tick() override {
        if (tickHandler_) {
            tWait_.update();
            tickHandler_(this);
        }
    }

    void draw(Canvas & c) override {
        aRot_.update();
        float angle = aRot_.interpolate(0.0, 360.0, Interpolation::Linear);
        c.drawTextureRotated(96, 56, sandClock_, angle, WHITE);
    }

    void onNavigationPush() override {
        aRot_.startContinuous();
    }

    Animation aRot_;
    Timer tWait_;
    Canvas::Texture sandClock_{"assets/icons/sand-clock.png"};


    std::function<void(Wait*)> tickHandler_;
}; // Wait