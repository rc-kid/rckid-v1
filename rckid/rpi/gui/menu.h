#pragma once


#include <vector>

#include "raylib_cpp.h"

class GUI;


class Menu {
public:

    class Item;

    Menu(std::initializer_list<Item> items):
        items_{items} {
    }

    size_t size() const { return items_.size(); }

    Item & operator [](size_t i) { return items_[i]; }

private:
    std::vector<Item> items_;    
    size_t i_ = 0;

}; // Menu



class Menu::Item {
public:    
    Item(std::string const & title, std::string const & imgFile):
        title_{title}, 
        imgFile_{imgFile} {
    }

    Texture2D const & img() const { return img_; }
    std::string const & title() const { return title_; }
    int titleWidth() const { return titleWidth_; }

    bool initialized() const { return titleWidth_ != UNINITIALIZED; }

    void initialize(GUI * gui);

protected:
    static constexpr int UNINITIALIZED = -1;

    std::string imgFile_;
    std::string title_;

    Texture2D img_;
    int titleWidth_ = UNINITIALIZED;

}; // MenuItem
