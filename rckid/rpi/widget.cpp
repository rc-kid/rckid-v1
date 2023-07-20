#include "window.h"
#include "widget.h"

void Widget::btnB(bool state) {
    if (state && ! window_->aswap_.running())
        window_->back();
}

void Widget::btnHome(bool state) {
    if (state && ! window_->aswap_.running())
        window_->setHomeMenu(); 
}


void ModalWidget::hide() {
    onBlur();
    window()->transition_ = Window::Transition::ModalOut;
    window()->aswap_.start();
}

void Dialog::draw() {
    window()->drawFrame(10, 30, 300, 180, "Foobar", BLUE);
}