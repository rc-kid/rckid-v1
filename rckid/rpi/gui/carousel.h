#pragma once

#include <vector>

#include "gui.h"
#include "menu.h"

/** The basic control widget for the RCKid UI
 
 */
class Carousel : public Widget {
public:

    Carousel(GUI * gui):
        Widget{gui} {
    }

    Menu * items() const { return items_; }

    size_t index() const { return i_; }

    void setItems(Menu * items, size_t index) {
        if (items_ == nullptr) {
            items_ = items;
            i_ = index;
            animation_ = Animation::FadeIn;
        } else {
            nextItems_ = items;
            nextI_ = index;
            animation_ = Animation::Swap;

        }
        frame_ = 0;
    }

protected:

    void drawItem(Menu::Item & item, int ximg, int xtext, uint8_t alpha = 255) {
        if (!item.initialized())
            item.initialize(gui());
        Color tint = ColorAlpha(WHITE, (float)alpha / 255);
        DrawTexture(item.img(), (GUI_WIDTH - item.img().width) / 2 + ximg, (GUI_HEIGHT - item.img().height - MENU_FONT_SIZE) / 2, tint);
        DrawTextEx(gui()->menuFont(), item.title().c_str(), (GUI_WIDTH - item.titleWidth()) / 2 + xtext, GUI_HEIGHT - FOOTER_HEIGHT - MENU_FONT_SIZE, MENU_FONT_SIZE, 1.0, tint);
    }

    void draw(double deltaMs) override {
        if (items_ == nullptr)
            return;
        switch (animation_) {
            case Animation::None: {
                drawItem(current(), 0, 0);
                return; // no need to close animation
            }
            case Animation::Left:
            case Animation::Right: {
                frame_ = std::min(MAX_FRAME, static_cast<int>(frame_ + deltaMs));
                double fpct = static_cast<double>(frame_) / MAX_FRAME;
                int imgi = interpolate(0, GUI_WIDTH, fpct);
                int texti = interpolate(0, GUI_WIDTH * 2, fpct);

                if (animation_ == Animation::Left) {
                    drawItem(next(), imgi, texti);
                    drawItem(current(),  - GUI_WIDTH + imgi, - GUI_WIDTH * 2 + texti);
                } else {
                    drawItem(prev(), - imgi, - texti);
                    drawItem(current(), GUI_WIDTH - imgi, GUI_WIDTH * 2 - texti);
                }
                break;
            }
            case Animation::FadeIn: {

                frame_ = std::min(MAX_FRAME, static_cast<int>(frame_ + deltaMs));
                double fpct = static_cast<double>(frame_) / MAX_FRAME;
                drawItem(current(), 0, 0);
                DrawRectangle(0, 0, GUI_WIDTH, GUI_HEIGHT, Fade(BLACK, 1 - fpct));
                break;
            }
            case Animation::FadeOut: {
                frame_ = std::min(MAX_FRAME, static_cast<int>(frame_ + deltaMs));
                double fpct = static_cast<double>(frame_) / MAX_FRAME;
                drawItem(current(), 0, 0);
                DrawRectangle(0, 0, GUI_WIDTH, GUI_HEIGHT, Fade(BLACK, fpct));
                if (frame_ == MAX_FRAME && nextItems_ != nullptr) {
                    animation_ = Animation::FadeIn;
                    frame_ = 0;
                    i_ = nextI_;
                    items_ = nextItems_;
                    nextItems_ = nullptr;
                    // TODO Detach & stuff? 
                }
            }
            case Animation::Swap: {
                frame_ = std::min(MAX_FRAME, static_cast<int>(frame_ + deltaMs));
                uint8_t alpha = interpolate(0, 255, frame_, MAX_FRAME) & 0xff;
                drawItem(current(), 0, 0, 255 - alpha);
                drawItem((*nextItems_)[nextI_], 0, 0, alpha);
                if (frame_ == MAX_FRAME) {
                    i_ = nextI_;
                    items_ = nextItems_;
                    nextItems_ = nullptr;
                    // TODO Detach & stuff? 
                }
            }
        }
        if (frame_ == MAX_FRAME) 
            animation_ = Animation::None;
    }

    void dpadLeft(bool state) override {
        if (state && animation_ == Animation::None && items_ != nullptr) {
            animation_ = Animation::Left;
            moveLeft();
            frame_ = 0;
        }
    }

    void dpadRight(bool state) override {
        if (state && animation_ == Animation::None && items_ != nullptr) {
            animation_ = Animation::Right;
            moveRight();
            frame_ = 0;
        }
    }

    Menu::Item & current() { return (*items_)[i_]; }
    Menu::Item & next() { return (*items_)[rightOf(i_) ]; }
    Menu::Item & prev() { return (*items_)[leftOf(i_) ]; }

    size_t leftOf(size_t i) { return i == 0 ? items_->size() - 1 : --i; }
    size_t rightOf(size_t i) { return i == items_->size() - 1 ? 0 : ++i; }

    size_t moveLeft() { return i_ = leftOf(i_); }
    size_t moveRight() { return i_ = rightOf(i_); }

private:

    Menu * items_ = nullptr;
    size_t i_ = 0;
    Menu * nextItems_ = nullptr;
    size_t nextI_ = 0;

    static constexpr int MAX_FRAME = 500;

    enum class Animation {
        Left, 
        Right,
        FadeIn,
        FadeOut,
        Swap,
        None,
    };
    Animation animation_ = Animation::None;
    int frame_ = 0;
}; // Carousel