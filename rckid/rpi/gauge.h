#pragma once

#include <functional>

#include "widget.h"
#include "window.h"

class Gauge : public Widget {
public:

    Gauge(Texture2D icon, int min, int max, int step, std::function<void(int)> onChange, std::function<void(Gauge*)> onRefresh):
        onChange_{onChange},
        onRefresh_{onRefresh},
        icon_{icon},
        min_{min},
        max_{max},
        step_{step}
    {
        gauge_ = LoadTexture("assets/gauge.png");
    }

    Gauge(Texture2D icon, int min, int max, int step, std::function<void(int)> onChange, int value):
        Gauge{icon, min, max, step, onChange, [](Gauge *g){ }} { value_ = value; }

    Gauge(std::string const & icon, int min, int max, int step, std::function<void(int)> onChange, std::function<void(Gauge*)> onRefresh):
        Gauge{LoadTexture(icon.c_str()), min, max, step, onChange, onRefresh} {}

    Gauge(std::string const & icon, int min, int max, int step, std::function<void(int)> onChange, int value):
        Gauge{LoadTexture(icon.c_str()), min, max, step, onChange, [](Gauge *g){}} { value_ = value; }

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
        DrawTexture(icon_, (Window_WIDTH - icon_.width) / 2, (Window_HEIGHT - icon_.height - MENU_FONT_SIZE) / 2 + 10, WHITE);

        DrawTexture(gauge_, 0, Window_HEIGHT - FOOTER_HEIGHT - MENU_FONT_SIZE + 5, backgroundColor_);
        if (max_ != min_) {
            BeginScissorMode(0, 0, 320 * (value_ - min_) / (max_ - min_), 240); 
            BeginBlendMode(BLEND_ALPHA);
            DrawTexture(gauge_, 0, Window_HEIGHT - FOOTER_HEIGHT - MENU_FONT_SIZE + 5, color_);
            EndBlendMode();
            EndScissorMode();
        }
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
    Color color_ = BLUE;
    Color backgroundColor_ = DARKGRAY;

    Texture2D icon_;
    Texture2D gauge_;

    std::function<void(int)> onChange_;
    std::function<void(Gauge*)> onRefresh_;

}; // Gauge