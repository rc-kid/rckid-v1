#include "window.h"
#include "widget.h"

void Widget::btnB(bool state) {
    if (state && ! window_->swap_.running())
        window_->back();
}

void Widget::btnHome(bool state) {
    if (state && ! window_->swap_.running())
        window_->setHomeMenu(); 
}
