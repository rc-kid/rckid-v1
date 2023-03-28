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
        frame_ = (frame_ + 5) % 320;
        DrawRectangle(0,0, frame_, frame_, GRAY);
        DrawTexture(currentImg_, (gui.widgetWidth() - currentImg_.width) / 2 + frame_, (gui.widgetHeight() - currentImg_.height) / 2, WHITE); 
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