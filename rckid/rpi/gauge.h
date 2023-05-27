#pragma once

#include <functional>

#include "widget.h"
#include "window.h"

class Gauge : public Widget {
public:

    Gauge(Window * window, std::string const & title, int min, int max, int step, std::function<void(int)> onChange, std::function<void(Gauge*)> onRefresh):
        Widget{window}, 
        onChange_{onChange},
        onRefresh_{onRefresh},
        title_{title},
        min_{min},
        max_{max},
        step_{step}
    {
        mask_ = LoadTexture("assets/gauge_mask.png");
        Vector2 fs = MeasureText(window->menuFont(), title_.c_str(), MENU_FONT_SIZE);
        titleWidth_ = fs.x;            
    }

    Gauge(Window * window, std::string const & title, int min, int max, int step, std::function<void(int)> onChange, int value):
        Gauge{window, title, min, max, step, onChange, [value](Gauge *g){ }} { value_ = value; }

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

    /** Calls the onRefresh method so that we can update the value when enabled */
    void onNavigationPush() override { onRefresh_(this); }

    void draw() override {
        int end = value_  > 0 ? 320 * (value_ - min_) / (max_ - min_) : 0;
        int gaugeTop = (Window_HEIGHT - mask_.height - MENU_FONT_SIZE) / 2;
        
        DrawRectangle(0, gaugeTop, 320, mask_.height, backgroundColor_);
        DrawRectangle(0, gaugeTop, end, mask_.height, color_);

        DrawTexture(mask_, (Window_WIDTH - mask_.width) / 2, gaugeTop, WHITE);

        DrawTextEx(window()->menuFont(), title_.c_str(), (Window_WIDTH - titleWidth_) / 2, Window_HEIGHT - FOOTER_HEIGHT - MENU_FONT_SIZE, MENU_FONT_SIZE, 1.0, WHITE);

    }

    void dpadLeft(bool state) override {
        if (state)
            setValue(value_ - step_);
    }

    void dpadRight(bool state) override {
        if (state)
            setValue(value_ + step_);
    }

    int min_ = 0;
    int max_ = 100;
    int step_ = 10;
    int value_ = 50;
    std::string title_{"Value"};
    int titleWidth_ = 0;
    Color color_ = BLUE;
    Color backgroundColor_ = DARKGRAY;

    Texture2D mask_;

    std::function<void(int)> onChange_;
    std::function<void(Gauge*)> onRefresh_;

}; // Gauge