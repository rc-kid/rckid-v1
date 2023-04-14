#include "window.h"

#include "animation.h"


bool Animation::update(Window * window) {
    if (!running_)
        return false;
    value_ += window->redrawDelta();
    if (value_ > duration_) {
        if (! continuous_) {
            running_ = false;
            value_ = duration_;
        } else {
            value_ -= duration_;
        }
        return true;
    } else {
        return false;
    }
}