#pragma once

#include <functional>

#include "widget.h"
#include "window.h"

class Gauge : public Widget {
public:
    Gauge(Window * window, std::function<void(int)> onChange): Widget{window}, onChange_{onChange} {}

    void setValue(int value) {
        if (value < min_)
            value = min_;
        else if (value > max_)
            value = max_;
        if (value_ != value) {
            value_ = value;
            if (onChange_)
                onChange_(value);
        }
    }

protected:

    void draw() override {
        if (titleWidth_ == 0) {
            mask_ = LoadTexture("assets/gauge_mask.png");
            Vector2 fs = MeasureText(window()->menuFont(), title_.c_str(), MENU_FONT_SIZE);
            titleWidth_ = fs.x;            
        }
        int end = value_  > 0 ? 320 * (value_ - min_) / (max_ - min_) : 0;
        int gaugeTop = (Window_HEIGHT - mask_.height - MENU_FONT_SIZE) / 2;
        
        DrawRectangle(0, gaugeTop, 320, mask_.height, backgroundColor_);
        DrawRectangle(0, gaugeTop, end, mask_.height, color_);

        DrawTexture(mask_, (Window_WIDTH - mask_.width) / 2, gaugeTop, WHITE);

        DrawTextEx(window()->menuFont(), title_.c_str(), (Window_WIDTH - titleWidth_) / 2, Window_HEIGHT - FOOTER_HEIGHT - MENU_FONT_SIZE, MENU_FONT_SIZE, 1.0, WHITE);

    }

    void dpadLeft(bool state) {
        if (state)
            setValue(value_ - step_);
    }

    void dpadRight(bool state) {
        if (state)
            setValue(value_ + step_);
    }

    int min_ = 0;
    int max_ = 100;
    int step_ = 10;
    int value_ = 50;
    std::string title_{"Brightness"};
    int titleWidth_ = 0;
    Color color_ = BLUE;
    Color backgroundColor_ = DARKGRAY;

    Texture2D mask_;

    std::function<void(int)> onChange_;

}; // Gauge