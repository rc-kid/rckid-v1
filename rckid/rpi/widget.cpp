#include "gui.h"

#include "widget.h"

GUIElement::~GUIElement() {
    gui_->detach(this);
}

GUIElement::GUIElement(GUI * gui):
    gui_{gui} {
    gui_->attach(this);
}