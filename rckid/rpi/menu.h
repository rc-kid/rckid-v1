#pragma once

#include <vector>
#include <functional>

#include "utils/json.h"

#include "raylib_cpp.h"
#include "widget.h"


class Window;
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

    void add(Item * item) { items_.push_back(item); }

    size_t size() const { return items_.size(); }

    Item * operator [](size_t i) { return items_[i]; }

protected:

    friend class Window;

    virtual void onFocus() {}

    virtual void onBlur() {}
    
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
    SubmenuItem(std::string const & title, std::string const & imgFile, std::initializer_list<Menu::Item *> items):
        Menu::Item{title, imgFile},
        submenu_{items} {
    }

    SubmenuItem(std::string const & title, std::string const & imgFile, std::function<void(Window *, SubmenuItem *)> updater):
        Menu::Item{title, imgFile},
        updater_{updater} {
    }

    Menu & submenu() { return submenu_; }

    void onSelect(Window * window) override;

private:
    std::function<void(Window *, SubmenuItem *)> updater_;
    Menu submenu_;
}; // SubmenuItem

/** Menu item that when selected opens a submenu read from JSON file. 
 
    The JSON file is expected to be an array of structs with the following fields:

    TODO 

    - title (string) - displayed title of the item
    - image (string) - path to an image that is to be used as the menu item image
    - kind (string) - one of json, music, video, game, 

    And some more
 */
class JSONItem : public Menu::Item {
public:
    JSONItem(std::string const & title, std::string const & imgFile, std::string const & jsonFile, Window * window):
        Item{title, imgFile},
        submenu_{jsonFile} {
    }

    Menu & submenu() { return submenu_; }

protected:

    class JSONMenu : public Menu {
    public:
        JSONMenu(std::string const & jsonFile): jsonFile_{jsonFile} {};

    protected:

        void onBlur() override { clear(); }

    private:

        friend class JSONItem;

        std::string jsonFile_;
        json::Value json_;

    }; 

    void onSelect(Window * window) override;

private:

    JSONMenu submenu_;
}; // JSONItem

class ActionItem : public Menu::Item {
public:
    ActionItem(std::string const & title, std::string const & imgFile, std::function<void()> action):
        Item{title, imgFile},
        action_{action} {
    }

    void onSelect(Window * window) override {
        if (action_)
            action_();
    }

private:
    std::function<void()> action_;
}; // ActionItem