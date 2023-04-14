#pragma once

#include "window.h"
#include "animation.h"

/** A simple pixel editor. 
 
    A simple pixel art editor for fun. Used dpad or stick to move around and then can set pixels to various colors. The whole thing can be saved. 
 */
class PixelEditor : public Widget {
public:

    static constexpr unsigned ICON_WIDTH = 64;
    static constexpr unsigned ICON_HEIGHT = 64;

    PixelEditor(Window * window):
        Widget{window}, 
        cursor_{500} {
        cursor_.startContinuous();
        memset(icon_, 0, sizeof(::Color) * ICON_WIDTH * ICON_HEIGHT);
    }

    bool fullscreen() const { return true; }

protected:

    void draw() override {
        cursor_.update(window());
        int pixelSize = std::min(Window_WIDTH / ICON_WIDTH, Window_HEIGHT / ICON_HEIGHT);
        int startx = (Window_WIDTH - (ICON_WIDTH * pixelSize)) / 2 ;
        int starty = (Window_HEIGHT - (ICON_HEIGHT * pixelSize)) / 2;
        for (int y = 0; y < ICON_HEIGHT; ++y) {
            for (int x = 0; x < ICON_WIDTH; ++x) {
               DrawRectangle(startx + x * pixelSize, starty + y * pixelSize, pixelSize, pixelSize, icon_[y * ICON_WIDTH + x]); 
            }
        }
        uint8_t c = cursor_.interpolateContinuous(0, 255, Interpolation::Linear); 
        DrawRectangleLines(startx - 1 + x_ * pixelSize, starty - 1 + y_ * pixelSize, pixelSize + 1, pixelSize + 1, ::Color{c, c, c, 255});
    }

    void dpadLeft(bool state) {
        if (state) 
            x_ = (--x_) % ICON_WIDTH;
    }

    void dpadRight(bool state) {
        if (state)
            x_ = (++x_) % ICON_WIDTH;
    }

    void dpadUp(bool state) {
        if (state)
            y_ = (--y_) % ICON_HEIGHT;
    }

    void dpadDown(bool state) {
        if (state)
            y_ = (++y_) % ICON_HEIGHT;
    }

    void btnX(bool state) {
        if (state)
            icon_[y_ * ICON_WIDTH + x_] = fg_;
    }

    void btnY(bool state) {
        if (state)
            icon_[y_ * ICON_WIDTH + x_] = bg_;
    }

    ::Color icon_[ICON_WIDTH * ICON_HEIGHT];
    ::Color fg_ = RED;
    ::Color bg_ = BLACK;
    unsigned x_ = 0;
    unsigned y_ = 0;
    Animation cursor_;


}; // PixelEditor