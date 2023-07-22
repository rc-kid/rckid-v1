#include "window.h"

#include "animation.h"


void Animation::update() {
    done_ = false;
    if (!running_)
        return;
    value_ += window().redrawDelta();
    if (value_ > duration_) {
        if (! continuous_) {
            running_ = false;
            value_ = duration_;
        } else {
            value_ -= duration_;
        }
        done_ = true;
    }
}

bool Timer::update() {
    if (!running_)
        return false;
    value_ += window().redrawDelta();
    if (value_ >= duration_) {
        if (singleShot_)
            running_ = false;
        else
            value_ -= duration_;
        return true;
    } else {
        return false;
    }    
}
