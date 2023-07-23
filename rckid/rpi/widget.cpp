#include "window.h"
#include "widget.h"

bool Widget::focused() const { return window().activeWidget() == this; }

void Widget::btnB(bool state) {
    if (state && ! window().aswap_.running())
        window().back();
}

void Widget::btnHome(bool state) {
    if (state && ! window().aswap_.running())
        window().setHomeMenu(); 
}

void Widget::setFooterHints() {
    window().resetFooter();
}