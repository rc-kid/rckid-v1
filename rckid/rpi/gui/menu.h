#pragma once

#include <vector>
#include <functional>

#include "raylib_cpp.h"

class GUI;
class Widget;

class Menu {
public:

    class Item;

    Menu() = default;

    Menu(std::initializer_list<Item*> items):
        items_{items} {
    }

    ~Menu() {
        clear();
    }

    void clear();

    size_t size() const { return items_.size(); }

    Item * operator [](size_t i) { return items_[i]; }

private:
    std::vector<Item*> items_;    
}; // Menu



class Menu::Item {
public:    
    Item(std::string const & title, std::string const & imgFile):
        title_{title}, 
        imgFile_{imgFile} {
    }

    // TODO should we remove the texture? 
    virtual ~Item() = default;

    Texture2D const & img() const { return img_; }
    std::string const & title() const { return title_; }
    int titleWidth() const { return titleWidth_; }

    bool initialized() const { return titleWidth_ != UNINITIALIZED; }

    void initialize(GUI * gui);

    /** Called by the carousel when the item is selected.
     */
    virtual void onSelect(GUI * gui) { }

protected:
    static constexpr int UNINITIALIZED = -1;

    friend class Carousel;


    std::string imgFile_;
    std::string title_;

    Texture2D img_;
    int titleWidth_ = UNINITIALIZED;

}; // MenuItem


/** A menu item behind which lies a widget to be displayed when selected. 
 */
class WidgetItem : public Menu::Item {
public:

    WidgetItem(std::string const & title, std::string const & imgFile, Widget * widget):
        Menu::Item{title, imgFile},
        widget_{widget} {
    }

    void onSelect(GUI * gui) override; 

private:

    Widget * widget_;
}; // WidgetItem

/** Menu item that launches its own submenu when selected. 
 */
class SubmenuItem : public Menu::Item {
public:
    SubmenuItem(std::string const & title, std::string const & imgFile, std::initializer_list<Menu::Item *> items):
        Menu::Item{title, imgFile},
        submenu_{items} {
    }

    SubmenuItem(std::string const & title, std::string const & imgFile, std::function<void(GUI *, SubmenuItem *)> updater):
        Menu::Item{title, imgFile},
        updater_{updater} {
    }

    Menu & submenu() { return submenu_; }

    void onSelect(GUI * gui) override;

private:
    std::function<void(GUI *, SubmenuItem *)> updater_;
    Menu submenu_;
}; // SubmenuItem

