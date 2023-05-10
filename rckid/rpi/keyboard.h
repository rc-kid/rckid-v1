#pragma once

#include "widget.h"
#include "window.h"
#include "animation.h"

/** On-screen keyboard for simple text input. 
 
    DPad moves between keys. A selects, B back, X Aa1, Y backspace
 */
class Keyboard : public Widget {
public:

    using OnDoneEvent = std::function<void(std::string)>;

    Keyboard(Window * window): Widget{window}, cursor_{500} {
        f_ = window->loadFont(MENU_FONT, KEY_HEIGHT);
        vf_ = window->loadFont(MENU_FONT, 40);
        cursor_.startContinuous();
    }

    std::string prompt;
    std::string value;
    OnDoneEvent onDone;

protected:

    static constexpr int KEY_WIDTH = 28;
    static constexpr int KEY_HEIGHT = 28;

    static constexpr int ROWS = 3;
    static constexpr int COLS = 10;

    void draw() override {

        cursor_.update(window());

        DrawTextEx(f_, prompt.c_str(), getButtonTopLeft(0,0).first, 30, 28, 1.0, GRAY);
        DrawRectangle(0, 65, 320, 40, ::Color{16, 16, 16, 255});
        DrawTextEx(vf_, value.c_str(), getButtonTopLeft(0,0).first, 65, 40, 1.0, WHITE);

        for (size_t i = 0; i < 30; ++i)
            drawButton(i % 10, i / 10, STR(KEYS[keyBank_][i]).c_str());

        uint8_t c = cursor_.interpolateContinuous(0, 255, Interpolation::Linear); 
        auto p = getButtonTopLeft(x_, y_);
        DrawRectangleLines(p.first, p.second, KEY_WIDTH, KEY_HEIGHT, ::Color{c, c, c, 255});
    }

    void drawButton(int x, int y, char const * text) {
        auto p = getButtonTopLeft(x, y);
        //x = 10 + x * 30;
        //y = 120 + y * 30;
        DrawRectangle(p.first, p.second, KEY_WIDTH,KEY_HEIGHT, RGB(30,30,30));
        Vector2 size = MeasureText(window()->menuFont(), text, KEY_HEIGHT);
        DrawTextEx(f_, text, p.first + (KEY_WIDTH - size.x) / 2, p.second, KEY_HEIGHT, 1.0, WHITE);
    }

    void onFocus() override {
        window()->addFooterItem(FooterItem::A("Enter"));
        window()->addFooterItem(FooterItem::X("Aa1"));
        window()->addFooterItem(FooterItem::Y("<-"));
    }


    void dpadLeft(bool state) {
        if (state) 
            x_ = (x_ == 0) ? (COLS - 1) : --x_ % COLS;
    }

    void dpadRight(bool state) {
        if (state)
            x_ = (++x_) % COLS;
    }

    void dpadUp(bool state) {
        if (state)
            y_ = (y_ == 0) ? (ROWS - 1) : --y_ % ROWS;
    }

    void dpadDown(bool state) {
        if (state)
            y_ = (++y_) % ROWS;
    }

    void btnX(bool state) {
        if (state)
            keyBank_ = (keyBank_ + 1) % 3;
    }

    void btnA(bool state) {
        if (state)
            value = value + KEYS[keyBank_][y_ * COLS + x_];
    }

    void btnY(bool state) {
        if (state && value.size() > 0)
            value.pop_back(); 
    }

    void btnStart(bool state) {
        if (onDone)
            onDone(value);
        window()->back();
    }

private:

    std::pair<int, int> getButtonTopLeft(int x, int y) {
        x = (320 - COLS * (KEY_WIDTH + 2)) / 2 + x * (KEY_WIDTH + 2);
        y = 120 + y * (KEY_HEIGHT + 2);
        return std::make_pair(x, y);
    }

    static inline char const * KEYS[] = {
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ.=_ ",
        "abcdefghijklmnopqrstuvwxyz,;- ",
        "0123456789!?~`@#$%^&*()+<>/|\\'\""
    };

    size_t keyBank_ = 0;
    int x_ = 0;
    int y_ = 0;    

    Animation cursor_;
    Font f_;
    Font vf_;

}; // Keyboard