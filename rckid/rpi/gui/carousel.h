#pragma once

#include "gui.h"

/** The basic widget for displaying RCKid's menu */
class Carousel : public Widget {
public:

    Carousel() {
        currentImg_ = LoadTexture("assets/images/001-game-controller.png");
        currentText_ = "Games";
    }

    void draw(GUI & gui) override {
        //frame_ = (frame_ + 5) % 320;
        //DrawRectangle(0,0, frame_, frame_, GRAY);
        DrawTexture(currentImg_, (GUI_WIDTH - currentImg_.width) / 2, (GUI_HEIGHT - currentImg_.height - MENU_FONT_SIZE) / 2, WHITE); 



        Vector2 fs = MeasureText(gui.menuFont(), currentText_.c_str(), MENU_FONT_SIZE);
        DrawTextEx(gui.menuFont(), currentText_.c_str(), (GUI_WIDTH - fs.x) / 2, GUI_HEIGHT - FOOTER_HEIGHT - MENU_FONT_SIZE, MENU_FONT_SIZE, 1.0, WHITE);

    }
private:

    // current image and text
    Texture2D currentImg_;
    std::string currentText_;
    // next image and text
    Texture2D nextImg_;    
    std::string nextText_;

    unsigned frame_ = 0;

}; // Carousel