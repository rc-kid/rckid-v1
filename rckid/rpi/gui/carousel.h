#pragma once

#include <vector>

#include "gui.h"

/** The basic control widget for the RCKid UI
 
 */
class Carousel : public Widget {
public:

    class Item {
    public:
        Item(std::string const & title, std::string const & imgFile):
            title_{title}, 
            imgFile_{imgFile} {
        }

        Texture2D const & img() const { return img_; }
        std::string const & title() const { return title_; }
    protected:
        friend class Carousel;

        static constexpr int UNINITIALIZED = -1;

        void initialize(GUI * gui) {
            img_ = LoadTexture(imgFile_.c_str());
            Vector2 fs = MeasureText(gui->menuFont(), title_.c_str(), MENU_FONT_SIZE);
            titleWidth_ = fs.x;
        }

        /** Draws the item. 
        */
        void draw(GUI * gui, int ximg, int xtext) {
            if (titleWidth_ == UNINITIALIZED)
                initialize(gui);
            DrawTexture(img_, (GUI_WIDTH - img_.width) / 2 + ximg, (GUI_HEIGHT - img_.height - MENU_FONT_SIZE) / 2, WHITE);
            DrawTextEx(gui->menuFont(), title_.c_str(), (GUI_WIDTH - titleWidth_) / 2 + xtext, GUI_HEIGHT - FOOTER_HEIGHT - MENU_FONT_SIZE, MENU_FONT_SIZE, 1.0, WHITE);
        }
        std::string imgFile_;
        std::string title_;

        Texture2D img_;
        int titleWidth_ = UNINITIALIZED;

    }; // Carousel::Item

    class Items {
    public:

        Items(std::initializer_list<Item> items):
            items_{items} {
        }

        size_t size() const { return items_.size(); }

        Item & operator [](size_t i) { return items_[i]; }


    private:



        friend class Carousel;

        std::vector<Item> items_;    
        size_t i_ = 0;


    }; // Carousel::Items

    Carousel(GUI * gui, Items * items = nullptr):
        Widget{gui},
        items_{items} {
        if (items_ != nullptr) {
            animation_ = Animation::FadeIn;
            frame_ = 0;
        }
    }

    Items * items() const { return items_; }

protected:

    void draw(double deltaMs) override {
        if (items_ == nullptr)
            return;
        switch (animation_) {
            case Animation::None: {
                current().draw(gui(), 0, 0);
                return; // no need to close animation
            }
            case Animation::Left:
            case Animation::Right: {
                frame_ = std::min(MAX_FRAME, static_cast<int>(frame_ + deltaMs));
                double fpct = static_cast<double>(frame_) / MAX_FRAME;
                int imgi = interpolate(0, GUI_WIDTH, fpct);
                int texti = interpolate(0, GUI_WIDTH * 2, fpct);

                if (animation_ == Animation::Left) {
                    next().draw(gui(), imgi, texti);
                    current().draw(gui(), - GUI_WIDTH + imgi, - GUI_WIDTH * 2 + texti);
                } else {
                    prev().draw(gui(), - imgi, - texti);
                    current().draw(gui(), GUI_WIDTH - imgi, GUI_WIDTH * 2 - texti);
                }
                break;
            }
            case Animation::FadeIn: {
                frame_ = std::min(MAX_FRAME, static_cast<int>(frame_ + deltaMs));
                double fpct = static_cast<double>(frame_) / MAX_FRAME;
                current().draw(gui(), 0, 0);
                DrawRectangle(0, 0, GUI_WIDTH, GUI_HEIGHT, Fade(BLACK, 1 - fpct));
                break;
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

    Item & current() { return (*items_)[i_]; }
    Item & next() { return (*items_)[rightOf(i_) ]; }
    Item & prev() { return (*items_)[leftOf(i_) ]; }

    size_t leftOf(size_t i) { return i == 0 ? items_->size() - 1 : --i; }
    size_t rightOf(size_t i) { return i == items_->size() - 1 ? 0 : ++i; }

    size_t moveLeft() { return i_ = leftOf(i_); }
    size_t moveRight() { return i_ = rightOf(i_); }

private:

    Items * items_;
    size_t i_ = 0;
    Items * nextItems_ = nullptr;

    static constexpr int MAX_FRAME = 500;

    enum class Animation {
        Left, 
        Right,
        FadeIn,
        None,
    };
    Animation animation_ = Animation::None;
    int frame_ = 0;
}; // Carousel