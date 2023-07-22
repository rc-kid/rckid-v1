#include "window.h"
#include "widget.h"

void Widget::btnB(bool state) {
    if (state && ! window().aswap_.running())
        window().back();
}

void Widget::btnHome(bool state) {
    if (state && ! window().aswap_.running())
        window().setHomeMenu(); 
}
