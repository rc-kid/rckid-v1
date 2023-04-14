#pragma once

#include "widget.h"
#include "gui.h"

/** On-screen keyboard for simple text input. 
 */
class Keyboard : public Widget {
public:

    Keyboard(GUI * gui): Widget{gui} {}

protected:
    void draw() override {
        DrawTextEx(gui()->helpFont(), prompt_.c_str(), 0, 20, 16, 1.0, GRAY);
        DrawRectangle(0, 40, 320, 40, DARKGRAY);

        for (size_t i = 0; i < 26; ++i)
            drawButton(i % 10, i / 10, STR(KEYS[i]).c_str());
    }

    void drawButton(int x, int y, char const * text) {
        x = 5 + x * 30;
        y = 85 + y * 30;
        DrawRectangle(x, y, 28, 28, RGB(30,30,30));
        Vector2 size = MeasureText(gui()->menuFont(), text, 28);
        DrawTextEx(gui()->menuFont(), text, x + (28 - size.x) / 2, y, 28, 1.0, WHITE);
    }

private:

    std::string prompt_{"Enter what you want:"};

    static inline char const * KEYS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    

}; // Keyboard