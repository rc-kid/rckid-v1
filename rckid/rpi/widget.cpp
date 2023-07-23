#include "window.h"
#include "widget.h"

bool Widget::focused() const { 
    Window * w = & window();
    return (w != nullptr && w->activeWidget() == this);
}

void Widget::btnB(bool state) {
    if (state && ! window().aswap_.running())
        window().back();
}

void Widget::btnHome(bool state) {
    if (state && ! window().aswap_.running())
        window().showHomeMenu(); 
}

void Widget::setFooterHints() {
    window().resetFooter();
}