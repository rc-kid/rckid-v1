#pragma once

#include "widget.h"

/** A simple single level menu. 

    Supports scrolling & stuff.  
 */
class Menu : public Widget {
public:

    Menu(std::initializer_list<std::string> menu):
        font_{window().canvas().defaultFont()} {
        for (auto m : menu) {
            menu_.push_back(m);
            int width = window().canvas().textWidth(m, font_);
            if (width > maxWidth_)
                maxWidth_ = width;
        }
        rowHeight_ = font_.size();
        maxRows_ = 190 / rowHeight_ * rowHeight_;
        if (maxRows_ > menu_.size())
            maxRows_ = menu_.size();
        cursor_.startContinuous();
    }


protected:

    void draw(Canvas & c) override {
        cursor_.update();
        BeginBlendMode(BLEND_ALPHA);
        c.setFont(font_);
        c.setFg(WHITE);
        int h = maxRows_ * font_.size() + 10;
        int start = 320 - 40 - maxWidth_;
        c.drawFrame(start, 220 - h, maxWidth_ + 30, h, PINK);
        // TODO change to other if we have more than maxROws rows
        size_t i = 0;
        for (size_t e = menu_.size(); i < e; ++i) {
            if (i == index_)
                c.drawText(start + 10, 220 - h + 5 + i * rowHeight_, menu_[i].c_str(), ColorBrightness(PINK, cursor_.interpolateContinuous(0.0, 1.0)));
            else
                c.drawText(start + 10, 220 - h + 5 + i * rowHeight_, menu_[i].c_str());
        }
    }

    void dpadUp(bool state) override {
        if (state)
            index_ > 0 ? --index_ : index_ = menu_.size() - 1;
    }

    void dpadDown(bool state) override {
        if (state) {
            ++index_;
            if (index_ == menu_.size())
                index_ = 0;
        }
    }

    void btnA(bool state) override {

    }

    void btnB(bool state) override {
        // TODO call the onCancel
        Widget::btnB(state);
    }

    Canvas::Font font_;
    std::vector<std::string> menu_;
    int maxWidth_{0};
    size_t maxRows_{0};
    size_t rowHeight_{0};
    size_t index_{0};
    Animation cursor_{500};

}; // Menu