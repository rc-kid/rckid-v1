#pragma once

#include "gui.h"

/** The basic control widget for the RCKid UI
 
 */
class Carousel : public Widget {
public:

    Carousel(GUI * gui):Widget{gui} {
        current_.set(gui, "assets/images/001-game-controller.png", "Games");
        next_.set(gui, "assets/images/002-rc-car.png","Remote");
    }

protected:

    void draw(double deltaMs) override {
       if (animation_ == Animation::None) {
            current_.draw(gui(), 0, 0);
       } else {
            frame_ = std::min(MAX_FRAME, static_cast<int>(frame_ + deltaMs));
            double fpct = static_cast<double>(frame_) / MAX_FRAME;
            int imgi = interpolate(0, GUI_WIDTH, fpct);
            int texti = interpolate(0, GUI_WIDTH * 2, fpct);

            if (animation_ == Animation::Left) {
                current_.draw(gui(), - imgi, - texti);
                next_.draw(gui(), GUI_WIDTH - imgi, GUI_WIDTH * 2 - texti);
            } else {
                current_.draw(gui(), imgi, texti);
                next_.draw(gui(), - GUI_WIDTH + imgi, - GUI_WIDTH * 2 + texti);
            }

            if (frame_ == MAX_FRAME) {
                std::swap(current_, next_);
                animation_ = Animation::None;
            }
       }
    }

    void btnL(bool state) override {
        if (state && animation_ == Animation::None) {
            animation_ = Animation::Left;
            frame_ = 0;
        }
    }

    void btnR(bool state) override {
        if (state && animation_ == Animation::None) {
            animation_ = Animation::Right;
            frame_ = 0;
        }
    }

private:

    struct Item {
        Texture2D img;
        std::string text;
        int textWidth;

        void set(GUI * gui, std::string const & imagePath, std::string const & title) {
            img = LoadTexture(imagePath.c_str());
            text = title;
            Vector2 fs = MeasureText(gui->menuFont(), text.c_str(), MENU_FONT_SIZE);
            textWidth = fs.x;
        }

        /** Draws the item. 
        */
        void draw(GUI * gui, int ximg, int xtext) {
            DrawTexture(img, (GUI_WIDTH - img.width) / 2 + ximg, (GUI_HEIGHT - img.height - MENU_FONT_SIZE) / 2, WHITE);
            DrawTextEx(gui->menuFont(), text.c_str(), (GUI_WIDTH - textWidth) / 2 + xtext, GUI_HEIGHT - FOOTER_HEIGHT - MENU_FONT_SIZE, MENU_FONT_SIZE, 1.0, WHITE);

        }
    }; // Carousel::Item

    Item current_;
    Item next_;

    static constexpr int MAX_FRAME = 500;


    enum class Animation {
        Left, 
        Right,
        None,
    };
    Animation animation_ = Animation::None;
    int frame_ = 0;
}; // Carousel