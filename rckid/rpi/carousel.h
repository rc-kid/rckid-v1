#pragma once

#include <vector>

#include "window.h"
#include "menu.h"
#include "animation.h"

/** The basic control widget for the RCKid UI
 
 */
class Carousel : public Widget {
public:

    Carousel(Window * window):
        Widget{window},
        animation_{500} {
        background_ = LoadTexture("assets/backgrounds/unicorns-black.png");
    }

    Menu * items() const { return items_; }

    size_t index() const { return i_; }

    void setItems(Menu * items, size_t index) {
        items_ = items;
        i_ = index;
        // cancel any ongoing animation we might have had
        animation_.stop();
    }

protected:

    void onFocus() override {
        window()->addFooterItem(FooterItem::A("Select"));
    }

    void dpadLeft(bool state) override {
        if (state && transition_ == Transition::None && items_ != nullptr) {
            transition_ = Transition::Left;
            moveLeft();
            animation_.start();
        }
    }

    void btnL(bool state) { dpadLeft(state); }

    void dpadRight(bool state) override {
        if (state && transition_ == Transition::None && items_ != nullptr) {
            transition_ = Transition::Right;
            moveRight();
            animation_.start();
        }
    }

    void btnR(bool state) { dpadRight(state); }

    void btnA(bool state) override {
        if (state && transition_ == Transition::None && items_ != nullptr)
            current()->onSelect(window());
    }

    void drawItem(Menu::Item * item, int ximg, int xtext, uint8_t alpha = 255) {
        if (!item->initialized())
            item->initialize(window());
        Color tint = ColorAlpha(WHITE, (float)alpha / 255);
        DrawTexture(item->img(), (Window_WIDTH - item->img().width) / 2 + ximg, (Window_HEIGHT - item->img().height - MENU_FONT_SIZE) / 2 + 10, tint);
        DrawTextEx(window()->menuFont(), item->title().c_str(), (Window_WIDTH - item->titleWidth()) / 2 + xtext, Window_HEIGHT - FOOTER_HEIGHT - MENU_FONT_SIZE, MENU_FONT_SIZE, 1.0, tint);
    }
    void drawBackground(int x) {
        int seam = adjustBackgroundSeam(backgroundSeam_ + x / 4.0);
        DrawTexture(background_, seam - 320, 0, ColorAlpha(WHITE, 0.3));
        DrawTexture(background_, seam, 0, ColorAlpha(WHITE, 0.3));
    }

    void draw() override {
        if (items_ == nullptr)
            return;
        if (animation_.update(window())) {
            backgroundSeam_ = adjustBackgroundSeam(backgroundSeam_ + ((transition_ == Transition::Left) ? 80 : -80));
            transition_ = Transition::None;
        }
        switch (transition_) {
            case Transition::None: {
                drawBackground(0);
                drawItem(current(), 0, 0);
                return; // no need to close animation
            }
            case Transition::Left:
            case Transition::Right: {
                int imgi = animation_.interpolate(0, Window_WIDTH);
                int texti = animation_.interpolate(0, Window_WIDTH * 2);
                if (transition_ == Transition::Left) {
                    drawBackground(imgi);
                    drawItem(next(), imgi, texti);
                    drawItem(current(),  - Window_WIDTH + imgi, - Window_WIDTH * 2 + texti);
                } else {
                    drawBackground(- imgi);
                    drawItem(prev(), - imgi, - texti);
                    drawItem(current(), Window_WIDTH - imgi, Window_WIDTH * 2 - texti);
                }
                break;
            }
            default:
                UNREACHABLE;
        }
    }

    Menu::Item * current() { return (*items_)[i_]; }
    Menu::Item * next() { return (*items_)[rightOf(i_) ]; }
    Menu::Item * prev() { return (*items_)[leftOf(i_) ]; }

    size_t leftOf(size_t i) { return i == 0 ? items_->size() - 1 : --i; }
    size_t rightOf(size_t i) { return i == items_->size() - 1 ? 0 : ++i; }

    size_t moveLeft() { return i_ = leftOf(i_); }
    size_t moveRight() { return i_ = rightOf(i_); }

private:

    int adjustBackgroundSeam(int value) {
        if (value > 320)
            value -= 320;
        else if (value < 0)
            value += 320;
        return value;
    }

    Menu * items_ = nullptr;
    size_t i_ = 0;

    enum class Transition {
        Left, 
        Right,
        None,
    };
    Transition transition_ = Transition::None;
    Animation animation_;

    Texture2D background_;
    int backgroundSeam_ = 160;
}; // Carousel