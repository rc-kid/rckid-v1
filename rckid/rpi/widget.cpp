#include "window.h"

#include "widget.h"

WindowElement::~WindowElement() {
    window_->detach(this);
}

WindowElement::WindowElement(Window * window):
    window_{window} {
    window_->attach(this);
}