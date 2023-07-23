#include "window.h"

#include "animation.h"


bool Animation::update() {
    done_ = false;
    if (!running_)
        return false;
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
    return done_;
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
