#pragma once

#include "carousel.h"

/** Carousel-style menu backed by a JSON object. 
 
    The JSON object must be an array of items, which are interpreted as items. Each item must be a JSON object that should contain the following:

    {
        "menu_title" : "Title text",
        "menu_icon" : "icon filename",
        "menu_subitems" : [], 
    }

    If the icon, or the title are missing a default value is displayed -- but missing on both is very likely an error. The item is either a leaf item, in which case the custom onItemSelected method of the carousel is called when the item is selected (button A), or can be a menu itself, which happens if 
 */
class BaseJSONCarousel : public BaseCarousel {
public:
    static inline std::string const MENU_TITLE{"menu_title"};
    static inline std::string const MENU_ICON{"menu_icon"};
    static inline std::string const MENU_SUBITEMS{"menu_subitems"};

    BaseJSONCarousel(json::Value root): root_{std::move(root)} {
        enter(& root_);
    }

protected:

    void itemSelected(size_t index) override {
        json::Value & item = (*json_.back())[MENU_SUBITEMS][index];
        if (item.containsKey(MENU_SUBITEMS))
            enter(& item);
        else 
            itemSelected(index, item);
    }

    virtual void itemSelected(size_t index, json::Value const & json) {}

    Item getItemFor(size_t index) override {
        auto const & item = (*json_.back())[MENU_SUBITEMS][index];
        json::Value const & jsonTitle = item[MENU_TITLE];
        std::string title{jsonTitle.isString() ? jsonTitle.value<std::string>() : defaultTitle_};    
        if (item.containsKey(MENU_ICON))
            return Item{title, item[MENU_ICON].value<std::string>()};
        else
            return Item{title};
    }

    void enter(json::Value * value) {
        json_.push_back(value);
        BaseCarousel::enter((*value)[MENU_SUBITEMS].size());
    }

    void leave() override {
        ASSERT(json_.size() > 1);
        json_.pop_back();
        BaseCarousel::leave();
    }

    std::string defaultTitle_{"???"};
    json::Value root_;
    std::vector<json::Value *> json_; 
}; // JSONCarousel

/** JSON backed carousel with a closure being called every time a leaf item is selected. 
 
    Useful for creating ad hoc json-backed menus. 
*/
class JSONCarousel : public BaseJSONCarousel {
public:
    JSONCarousel(json::Value json, std::function<void(json::Value const &)> onSelect):
        BaseJSONCarousel{json},
        onSelect_{onSelect} {
    }

    static json::Value item(std::string const & title, std::string const & icon, std::string const & fields) {
        json::Value result = json::parse(fields);
        result.insert(MENU_TITLE, json::Value{title});
        result.insert(MENU_ICON, json::Value{icon});
        return result;
    }

    static json::Value item(std::string const & title, std::string const & fields) {
        json::Value result = json::parse(fields);
        result.insert(MENU_TITLE, json::Value{title});
        return result;
    }

    static json::Value menu(std::string const & title, std::string const & icon, std::initializer_list<json::Value> subitems) {
        json::Value result = json::Value::newStruct();
        json::Value items = json::Value::newArray();
        for (auto i : subitems)
            items.push(i);
        result.insert(MENU_TITLE, json::Value{title});
        result.insert(MENU_ICON, json::Value{icon});
        result.insert(MENU_SUBITEMS, items);
        return result;
    }

private:

    void itemSelected(size_t index, json::Value const & json) override { onSelect_(json); }

    std::function<void(json::Value const &)> onSelect_;
}; // CustomJSONCarousel
