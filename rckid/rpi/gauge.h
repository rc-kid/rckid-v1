#pragma once

#include <functional>

#include "widget.h"
#include "window.h"

class Gauge : public Widget {
public:

    Gauge(Canvas::Texture const & icon, int min, int max, int step, std::function<void(int)> onChange, std::function<void(Gauge*)> onRefresh):
        onChange_{onChange},
        onRefresh_{onRefresh},
        icon_{icon},
        min_{min},
        max_{max},
        step_{step},
        gauge_{"assets/gauge.png"} {
    }

    Gauge(Canvas::Texture const & icon, int min, int max, int step, std::function<void(int)> onChange, int value):
        Gauge{icon, min, max, step, onChange, [](Gauge *g){ }} { value_ = value; }

    Gauge(std::string const & icon, int min, int max, int step, std::function<void(int)> onChange, std::function<void(Gauge*)> onRefresh):
        Gauge{Canvas::Texture{icon}, min, max, step, onChange, onRefresh} {}

    Gauge(std::string const & icon, int min, int max, int step, std::function<void(int)> onChange, int value):
        Gauge{Canvas::Texture{icon}, min, max, step, onChange, [](Gauge *g){}} { value_ = value; }

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

    void draw(Canvas & c) override {
        c.drawTexture((Window_WIDTH - icon_.width()) / 2, (Window_HEIGHT - icon_.height() - MENU_FONT_SIZE) / 2 + 5, icon_);

        c.drawTexture(0, Window_HEIGHT - FOOTER_HEIGHT - MENU_FONT_SIZE + 5, gauge_, backgroundColor_);
        if (max_ != min_) {
            BeginScissorMode(0, 0, 320 * (value_ - min_) / (max_ - min_), 240); 
            BeginBlendMode(BLEND_ALPHA);
            c.drawTexture(0, Window_HEIGHT - FOOTER_HEIGHT - MENU_FONT_SIZE + 5, gauge_, color_);
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

    Canvas::Texture icon_;
    Canvas::Texture gauge_;

    std::function<void(int)> onChange_;
    std::function<void(Gauge*)> onRefresh_;

}; // Gauge