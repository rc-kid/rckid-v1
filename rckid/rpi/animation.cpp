#include "window.h"

#include "animation.h"


void Animation::update(Window * window) {
    done_ = false;
    if (!running_)
        return;
    value_ += window->redrawDelta();
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