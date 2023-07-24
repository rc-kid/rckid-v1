#pragma once

#include "widget.h"

class Menu : public Widget {
public:

    Menu(std::initializer_list<std::string> menu):
        font_{window().headerFont()} {
        for (auto m : menu) {
            menu_.push_back(m);
            int width = MeasureTextEx(font_, m.c_str(), font_.baseSize, 1.0).x;
            if (width > maxWidth_)
                maxWidth_ = width;
        }
    }


protected:

    void draw() override {
        int h = 70;

        window().drawFrame(20, 220 - h, maxWidth_ + 10, h, "Menu", WHITE);
    }

    void dpadUp(bool state) override {

    }

    void dpadDown(bool state) override {

    }

    void btnA(bool state) override {

    }

    void btnB(bool state) override {
        // TODO call the onCancel
        Widget::btnB(state);
    }

    Font font_;
    std::vector<std::string> menu_;
    int maxWidth_ = 0;
    size_t index_ = 0;

}; // Menu