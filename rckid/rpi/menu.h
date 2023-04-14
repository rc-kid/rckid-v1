#pragma once

#include <vector>
#include <functional>

#include "raylib_cpp.h"
#include "widget.h"


class Window;
class Widget;

class Menu : public WindowElement {
public:

    class Item;

    Menu(Window * window): WindowElement{window} {};

    Menu(Window * window, std::initializer_list<Item*> items):
        WindowElement{window}, 
        items_{items} {
    }

    ~Menu() {
        clear();
    }

    void clear();

    size_t size() const { return items_.size(); }

    Item * operator [](size_t i) { return items_[i]; }

protected:
    
    void onRenderingPaused() override;

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

    void initialize(Window * window);

    /** Called by the carousel when the item is selected.
     */
    virtual void onSelect(Window * window) { }

protected:
    static constexpr int UNINITIALIZED = -1;

    friend class Carousel;
    friend class Menu;

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

    void onSelect(Window * window) override; 

private:

    Widget * widget_;
}; // WidgetItem

/** Menu item that launches its own submenu when selected. 
 */
class SubmenuItem : public Menu::Item {
public:
    SubmenuItem(std::string const & title, std::string const & imgFile, Window * window, std::initializer_list<Menu::Item *> items):
        Menu::Item{title, imgFile},
        submenu_{window, items} {
    }

    SubmenuItem(std::string const & title, std::string const & imgFile,Window * window, std::function<void(Window *, SubmenuItem *)> updater):
        Menu::Item{title, imgFile},
        submenu_{window},
        updater_{updater} {
    }

    Menu & submenu() { return submenu_; }

    void onSelect(Window * window) override;

private:
    std::function<void(Window *, SubmenuItem *)> updater_;
    Menu submenu_;
}; // SubmenuItem

