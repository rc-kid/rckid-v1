#pragma once

#include "gui.h"

/** A simple pixel editor. 
 
    A simple pixel art editor for fun. Used dpad or stick to move around and then can set pixels to various colors. The whole thing can be saved. 
 */
class PixelEditor : public Widget {
public:

    static constexpr unsigned ICON_WIDTH = 64;
    static constexpr unsigned ICON_HEIGHT = 64;

    PixelEditor(GUI * gui): Widget{gui} {
        memset(icon_, 0, sizeof(::Color) * ICON_WIDTH * ICON_HEIGHT);
    }

protected:

    static constexpr int MAX_FRAME = 500;

    void draw(double deltaMs) override {
        frame_ = static_cast<int>(frame_ + deltaMs) % MAX_FRAME;
        int pixelSize = std::min(GUI_WIDTH / ICON_WIDTH, GUI_HEIGHT / ICON_HEIGHT);
        int startx = (GUI_WIDTH - (ICON_WIDTH * pixelSize)) / 2 ;
        int starty = (GUI_HEIGHT - (ICON_HEIGHT * pixelSize)) / 2;
        for (int y = 0; y < ICON_HEIGHT; ++y) {
            for (int x = 0; x < ICON_WIDTH; ++x) {
               DrawRectangle(startx + x * pixelSize, starty + y * pixelSize, pixelSize, pixelSize, icon_[y * ICON_WIDTH + x]); 
            }
        }
        uint8_t c = interpolateContinous(0, 255, frame_, MAX_FRAME, Interpolation::Linear) & 0xff;
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
    int frame_ = 0;


}; // PixelEditor