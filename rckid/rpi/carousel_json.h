#pragma once

#include <filesystem>

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
    }

protected:

    void reset() override {
        json_.clear();
        BaseCarousel::reset();
        enter(& root_);
    }

    void itemSelected(size_t index) override {
        json::Value & item = currentJson()[MENU_SUBITEMS][index];
        if (item.containsKey(MENU_SUBITEMS))
            enter(& item);
        else 
            itemSelected(index, item);
    }

    virtual void itemSelected(size_t index, json::Value const & json) {}

    Item getItemFor(size_t index) override {
        auto const & item = currentJson()[MENU_SUBITEMS][index];
        json::Value const & jsonTitle = item[MENU_TITLE];
        std::string title{jsonTitle.isString() ? jsonTitle.value<std::string>() : defaultTitle_};    
        if (item.containsKey(MENU_ICON))
            return Item{title, item[MENU_ICON].value<std::string>()};
        else
            return Item{title};
    }

    virtual void enter(json::Value * value) {
        json_.push_back(value);
        BaseCarousel::enter((*value)[MENU_SUBITEMS].size());
    }

    void leave() override {
        ASSERT(json_.size() > 1);
        json_.pop_back();
        BaseCarousel::leave();
    }

    json::Value & currentJson() { return *json_.back(); }

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

protected:

    void itemSelected(size_t index, json::Value const & json) override { onSelect_(json); }

    std::function<void(json::Value const &)> onSelect_;
}; // SONCarousel

/**
 
    Commands:
    - create new folder
    - rename stuff
    - move left / right
    - move up
    - mode down () ? 
 */

class DirSyncedCarousel : public BaseJSONCarousel {
public:

    DirSyncedCarousel(std::string const & rootDir):
        BaseJSONCarousel{getOrCreateItemsJSON(rootDir)}, 
        rootDir_{rootDir} {
    }

protected:
    static inline std::string const MENU_FILENAME{"menu_filename"};

    using DirEntry = std::filesystem::directory_entry;
    using DirIterator = std::filesystem::directory_iterator;
    using Path = std::filesystem::path;

    void enter(json::Value * value) {
        if (value == & root_)
            currentDir_ = rootDir_;
        else
            currentDir_ = currentDir_.append((*value)[MENU_FILENAME].value<std::string>());
        // sync before calling the json's enter so that we are entering the correct json
        syncWithFolder(*value, currentDir_);
        BaseJSONCarousel::enter(value);
    }

    void leave() {
        BaseJSONCarousel::leave();
        currentDir_ = currentDir_.parent_path();
        // sync on our way back as well if there was move up 
        // TODO make this conditional on move up flag
        syncWithFolder(currentJson(), currentDir_);
    }

    void reset() override {
        currentDir_.clear();
        //syncWithFolder(root_, rootDir_);
        BaseJSONCarousel::reset();
    }

    virtual std::optional<json::Value> getItemForFile(DirEntry const & entry) {
        json::Value item{json::Value::newStruct()};
        item.insert(MENU_FILENAME, entry.path().filename().c_str());
        item.insert(MENU_TITLE, entry.path().stem().c_str());
        return item;
    }

    /** Whenever a folder is opened, the folder's contents is synchronized with the items stored in the JSON. 
     */
    void syncWithFolder(json::Value & json, Path const & folder) {
        // first create a map of items in the JSON for fast access
        TraceLog(LOG_INFO, STR("Syncing directory " << folder));
        std::unordered_map<std::string, json::Value *> inJson;
        for (auto & i : json[MENU_SUBITEMS].arrayElements()) {
            std::string name = i[MENU_FILENAME].value<std::string>();
            inJson.insert(std::make_pair(name, & i)); 
        }
        TraceLog(LOG_INFO, STR("  in json items found: " << inJson.size()));
        for (auto const & entry : DirIterator{folder}) {
            std::string filename = entry.path().filename();
            auto i = inJson.find(filename);
            if (i != inJson.end()) {
                // TODO only do continue if its file/file or dir/dir
                continue;
            }
            // not found, or trying to add anyways: if it's a directory, add an empty submenu that will be filled later with its own sync
            if (entry.is_directory()) {
                json::Value d{json::Value::newStruct()};
                d.insert(MENU_TITLE, entry.path().stem().c_str());
                d.insert(MENU_ICON, "");
                d.insert(MENU_FILENAME, entry.path().filename().c_str());
                d.insert(MENU_SUBITEMS, json::Value::newArray());
                json[MENU_SUBITEMS].push(d);
            } else {
                auto i =  getItemForFile(entry);
                if (i) {
                    TraceLog(LOG_INFO, STR("  added file " << entry.path().filename()));
                    json[MENU_SUBITEMS].push(i.value());
                }
            }
        }
    }

    static json::Value getOrCreateItemsJSON(std::string const & dir) {
        std::string itemsFile = dir + "/items.json";
        try {
            return json::parseFile(itemsFile);
        } catch (std::exception const & e) {
            TraceLog(LOG_ERROR, STR("Error reading JSON file " << itemsFile << ": " << e.what()));
            // and create an empty JSON
            json::Value result{json::Value::newStruct()};
            result.insert(MENU_TITLE, json::Value{""});
            result.insert(MENU_ICON, json::Value{""});
            result.insert(MENU_SUBITEMS, json::Value::newArray());
            return result;
        }
    }

    Path rootDir_;
    Path currentDir_;

}; 