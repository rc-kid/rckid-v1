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
    Menu(std::initializer_list<Item*> items): items_{items} { }

    virtual ~Menu() { clear(); }

    void clear();

    void add(Item * item) { items_.push_back(item); }

    size_t size() const { return items_.size(); }

    Item * operator [](size_t i) { return items_[i]; }

protected:

    friend class Window;
    friend class Carousel;

    virtual void onFocus() {}

    virtual void onBlur() {}

    /** Called when an item in the menu is selected. Default implementation simply calls the item's onSelect event. 
     
        Overriding this methods allows the menu to intercept their items being selected. 
     */
    virtual void onSelectItem(Window * window, size_t index);
    
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

/** Menu item that calls its own closure when the selected. 
 */
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


/** A menu item behind which lies a widget to be displayed when selected. 
 
    Semantically corresponds to a RAII holder of the widget combined with an ActionItem whose action is to push the widget on the nav stack. 
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
class Submenu : public Menu::Item {
public:
    Submenu(std::string const & title, std::string const & imgFile, std::initializer_list<Menu::Item *> items):
        Menu::Item{title, imgFile},
        submenu_{new Menu{items}} {
    }

    Submenu(std::string const & title, std::string const & imgFile, Menu * submenu):
        Menu::Item{title, imgFile},
        submenu_{submenu} {
    }

    ~Submenu() override { delete submenu_; }

    Menu * submenu() { return submenu_; }

    void setSubmenu(Menu * value) {
        delete submenu_;
        submenu_ = value;
    }

    void onSelect(Window * window) override;

    Menu * submenu_ = nullptr;
}; // Submenu


/** Menu item that launches a submenu when selected. 
 
    Unlike the Submenu class the LazySubmenu will construct the menu first via the provided action method. The returned menu will then be cached for future invocations. 
*/
class LazySubmenu : public Submenu {
public:
    LazySubmenu(std::string const & title, std::string const & imgFile, std::function<Menu *(Window *)> updater):
        Submenu{title, imgFile, nullptr},
        updater_{updater} {
    }

    void onSelect(Window * window) override {
        if (submenu() == nullptr)
            setSubmenu(updater_(window));
        Submenu::onSelect(window);
    }

private:
    std::function<Menu*(Window *)> updater_;
}; // LazySubmenu

/** Menu backed by a json file. 
 
    The JSON file must be an array of JSON objects that contain at least the following keys:

    - title (string) - displayed title of the item
    - image (string) - path to an image that is to be used as the menu item image

 */
class JSONMenu : public Menu {
public:
    JSONMenu(std::string const & jsonFile, std::function<void(Window *, json::Value &)> onSelect):
        onSelect_{onSelect},
        jsonFile_{jsonFile}, 
        json_{json::parseFile(jsonFile)} {
        for (json::Value & item : json_.arrayElements()) {
            if (item.containsKey("json")) {
                std::string jsonPath = item["json"].value<std::string>();
                add(new LazySubmenu{
                    item["title"].value<std::string>(), 
                    item["image"].value<std::string>(), 
                    [jsonPath, onSelect](Window *){ 
                        return new JSONMenu{jsonPath, onSelect}; 
                    }
                });
            } else {
                add(new Menu::Item{item["title"].value<std::string>(), item["image"].value<std::string>()});
            }
        }
    }

protected:

    void onSelectItem(Window * window, size_t index) {
        json::Value & v = json_[index];
        if (v.containsKey("json"))
            (*this)[index]->onSelect(window);
        else
            onSelect_(window, v);   
    }

private:

    std::function<void(Window *, json::Value &)> onSelect_;

    std::string jsonFile_;
    json::Value json_;

}; // JSONMenu





/** Menu item that when selected opens a submenu read from JSON file. 
 
    The JSON file is expected to be an array of structs with the following fields:

    TODO 

    - title (string) - displayed title of the item
    - image (string) - path to an image that is to be used as the menu item image
    - kind (string) - one of json, music, video, game, 

    And some more
 */
/*
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
*/
