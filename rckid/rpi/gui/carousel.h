#pragma once

#include <vector>

#include "gui.h"
#include "menu.h"
#include "animation.h"

/** The basic control widget for the RCKid UI
 
 */
class Carousel : public Widget {
public:

    Carousel(GUI * gui):
        Widget{gui}, 
        animation_{500} {
    }

    Menu * items() const { return items_; }

    size_t index() const { return i_; }

    void setItems(Menu * items, size_t index) {
        if (items_ == nullptr) {
            items_ = items;
            i_ = index;
            transition_ = Transition::FadeIn;
        } else {
            nextItems_ = items;
            nextI_ = index;
            transition_ = Transition::Swap;

        }
        animation_.start();
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
        if (animation_.update(deltaMs)) {
            if (transition_ == Transition::Swap) {
                // TODO detach?
                i_ = nextI_;
                items_ = nextItems_;
                nextItems_ = nullptr;
            }
            transition_ = Transition::None;
        }
        switch (transition_) {
            case Transition::None: {
                drawItem(current(), 0, 0);
                return; // no need to close animation
            }
            case Transition::Left:
            case Transition::Right: {
                int imgi = animation_.interpolate(0, GUI_WIDTH);
                int texti = animation_.interpolate(0, GUI_WIDTH * 2);
                if (transition_ == Transition::Left) {
                    drawItem(next(), imgi, texti);
                    drawItem(current(),  - GUI_WIDTH + imgi, - GUI_WIDTH * 2 + texti);
                } else {
                    drawItem(prev(), - imgi, - texti);
                    drawItem(current(), GUI_WIDTH - imgi, GUI_WIDTH * 2 - texti);
                }
                break;
            }
            case Transition::FadeIn: {
                drawItem(current(), 0, 0, animation_.interpolate(0, 255));
                break;
            }
            case Transition::FadeOut: {
                drawItem(current(), 0, 0, animation_.interpolate(255, 0));
                break;
            }
            case Transition::Swap: {
                uint8_t alpha = animation_.interpolate(0, 255) & 0xff;
                drawItem(current(), 0, 0, 255 - alpha);
                drawItem((*nextItems_)[nextI_], 0, 0, alpha);
                break;
            }
            default:
                UNREACHABLE;
        }
    }

    void dpadLeft(bool state) override {
        if (state && transition_ == Transition::None && items_ != nullptr) {
            transition_ = Transition::Left;
            moveLeft();
            animation_.start();
        }
    }

    void dpadRight(bool state) override {
        if (state && transition_ == Transition::None && items_ != nullptr) {
            transition_ = Transition::Right;
            moveRight();
            animation_.start();
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

    enum class Transition {
        Left, 
        Right,
        FadeIn,
        FadeOut,
        Swap,
        None,
    };
    Transition transition_ = Transition::None;
    Animation animation_;
}; // Carousel