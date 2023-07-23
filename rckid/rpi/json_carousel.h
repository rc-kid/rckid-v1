#pragma once

#include <filesystem>
#include <vector>
#include <functional>
#include <memory>

#include "utils/json.h"

#include "widget.h"
#include "animation.h"

/** Basic carousel functionality and features common for various carousel types. 

    - does the item rendering, and others
    - keeps the  
 
*/
class BaseCarousel : public Widget {
protected:

    BaseCarousel():
        defaultIcon_{LoadTexture("assets/images/008-unicorn.png")} {
    }

    /** Item information consisting of the image and text to display with the item. 
     */
    class Item {
    public:
        Item(std::string const & text, Texture icon):
            icon{icon}, text{text} {
        }

        Item(std::string const & text, std::string const & iconFile):
            icon{LoadTexture(iconFile.c_str())},
            text{text} {
        }

        Item(std::string const & text): text{text} {
            icon.id = 0;
        }

        Item(Item const &) = delete;

        Item(Item && from): icon{from.icon}, text{std::move(from.text)} {
            from.icon.id = 0;
        }

        ~Item() {
            if (!useDefaultIcon())
                UnloadTexture(icon);
        }

        bool useDefaultIcon() const { return icon.id == 0; }

        Texture icon;
        std::string text;

    }; 

    bool busy() const { return a_.running(); }

    /** Creates the information about the index-th item. 
     
        The item consists of the icon and the title and must be implemented in the data providing subclasses. 
     */
    virtual Item getItemFor(size_t index) = 0;

    /** What to do when an item is selected. 
     */
    virtual void itemSelected(size_t index) {}

    virtual void goToPrev() {
        pos().current = pos().prev();
        startTransition(Transition::Prev);
    }

    virtual void goToNext() {
        pos().current = pos().next();
        startTransition(Transition::Next);
    }

    virtual void enter(size_t numItems) {
        pos_.emplace_back(numItems);
        startTransition(pos_.size() == 1 ? Transition::FadeIn : Transition::EnterFadeOut);
    }

    virtual void leave() {
        if (pos_.size() > 1) {
            startTransition(Transition::LeaveFadeOut);
        } else {
            // TODO window's nav pane go back
        }
    }

    void setFooterHints() override {
        // don't call the Widget's set footer as we do the reset ourselves
        window().resetFooter(/* forceBack */ pos_.size() > 1);
        window().addFooterItem(FooterItem::A("Select"));
    }


    void draw() override {
        a_.update();
        switch (t_) {
            case Transition::None:
                drawItem(getItem(pos().current), 0, 255);
                cancelRedraw();
                break;
            // when the transition is to the left item, the current item is on the left and we are moving right, i.e. current item's offset moves from -320 to 0 and the rightOf (previously current) moves from 0 to 320. The animation moves to the *right*
            case Transition::Prev: {
                int offset = a_.interpolate(0, Window_WIDTH);
                window().setBackgroundSeam(seamStart_ + offset / 4);
                drawItem(getItem(pos().next()), offset, 255);
                drawItem(getItem(pos().current), offset - Window_WIDTH, 255);   
                break;
            }
            // for right transition, the current item is on the right and we move to the left, i.e. the leftOf item starts at 0 and goes to -320, while the current item starts at 320 and goes to 0
            case Transition::Next: {
                int offset = a_.interpolate(0, Window_WIDTH);
                window().setBackgroundSeam(seamStart_ - offset / 4);
                drawItem(getItem(pos().prev()), - offset, 255);
                drawItem(getItem(pos().current), Window_WIDTH - offset, 255);                
                break;
            }
            case Transition::EnterFadeOut: {
                int alpha = a_.interpolate(255, 0);
                drawItem(posPrev().items[posPrev().current].get(), 0, alpha);
                if (a_.done())
                    startTransition(Transition::FadeIn);
                break;
            }
            case Transition::LeaveFadeOut: {
                int alpha = a_.interpolate(255, 0);
                drawItem(pos().items[pos().current].get(), 0, alpha);
                if (a_.done()) {
                    pos_.pop_back();
                    startTransition(Transition::FadeIn);
                }
                break;
            }
            case Transition::FadeIn: {
                int alpha = a_.interpolate(0, 255);
                drawItem(getItem(pos().current), 0, alpha);
                break;
            }
            // TODO in & out
        }
        if (a_.done())
            t_ = Transition::None;             
    }

    void btnA(bool state) override {
        if (state && !busy())
            itemSelected(pos().current);
    }

    void btnB(bool state) override {
        if (state && !busy())
            leave();
    }

    void dpadLeft(bool state) override {
        if (state && !busy())
            goToPrev();
    }

    void dpadRight(bool state) override {
        if (state && !busy())
            goToNext();
    }

private:

    /** Currently animated transition. 
     */
    enum class Transition {
        Prev, 
        Next,
        EnterFadeOut,
        LeaveFadeOut,
        FadeIn, 
        None, 
    }; 

    /** The cached item consists of the item returned by the subclasses and precalculated information about the item's position */
    struct CachedItem : public Item {
        int iX;
        int iY;
        float iScale = 1.0;

        Font font;
        int textWidth;
        int tX;
        int tY;
        bool alphaBlend = false;

        CachedItem(Item && item):
            Item{std::move(item)} {
            if (useDefaultIcon()) {
                iX = 96;
                iY = 24;
            } else {
                if (icon.height <= 128) {
                    iY = 88 - icon.height / 2;
                } else  {
                    // the text might cover some of the icon, set alpha blending instead of the default additive blending that would be used over transparent background
                    alphaBlend = true;
                    if (icon.height > 240)
                        iScale = 240.f / icon.height;
                    else
                        iY = (240 - icon.height) / 2;
                }
                if (icon.width <= 320) {
                    iX = (320 - icon.width) / 2;
                } else {
                    float s = 320.f / icon.width;
                    if (s < iScale)
                        iScale = s;
                }    
                if (iScale != 1.0) {
                    iY = static_cast<int>((240 - icon.height * iScale) / 2);
                    iX = static_cast<int>((320 - icon.width * iScale) / 2);
                }
            }
            font = window().menuFont();
            textWidth = MeasureTextEx(font, text.c_str(), font.baseSize, 1.0).x;
            tY = 152;
            tX = 160 - (textWidth / 2);
            // TODO calculate where to draw the text & the icon and other things
        }

        CachedItem(CachedItem const &) = delete;

    }; // Carousel::CachedItem

    /** Current position of the carousel. 
     
        The position consists of the index of the current item and the number of items available. 
     */
    struct CachedPosition {
        size_t numItems;
        size_t current;
        std::vector<std::unique_ptr<CachedItem>> items;

        CachedPosition(size_t numItems):
            numItems{numItems},
            current{0} {
            items.resize(numItems);
        }

        CachedPosition(CachedPosition &&) = default;

        CachedPosition(CachedPosition const &) = delete;

        size_t prev() const { return current != 0 ? current - 1 : numItems - 1; }
        size_t next() const { return current != numItems - 1 ? current + 1 : 0; }
    }; 


    /** Returns the current position in the stack. 
     */
    CachedPosition & pos() {
        ASSERT(! pos_.empty());
        return pos_.back();
    }

    CachedPosition & posPrev() {
        ASSERT(pos_.size() >= 2);
        return pos_[pos_.size() - 2];
    }

    CachedItem * getItem(size_t index) {
        CachedPosition & p = pos();
        if (p.items[index] == nullptr)
            p.items[index] = std::unique_ptr<CachedItem>{new CachedItem{getItemFor(index)}};
        return p.items[index].get();
    }

    void startTransition(Transition t) {
        requestRedraw();
        seamStart_ = window().backgroundSeam();
        t_ = t;
        switch (t) {
            case Transition::Prev:
            case Transition::Next:
                a_.start(500);
                return;
            case Transition::FadeIn:
                if (focused()) {
                    setFooterHints();
                    window().showFooter();
                }
                break;
            case Transition::EnterFadeOut:
            case Transition::LeaveFadeOut:
                window().hideFooter();
                break;
        }
        a_.start(250);
    }

    /** Draws the previously created item
     */
    void drawItem(CachedItem * item, int offset, uint8_t alpha) {
        if (item == nullptr)
            return;
        Color c{RGBA(255, 255, 255, alpha)};
        if (item->iScale == 1.0)
            DrawTexture(item->useDefaultIcon() ? defaultIcon_ : item->icon, item->iX + offset, item->iY, c);
        else
            DrawTextureEx(item->icon, V2(item->iX + offset, item->iY), 0, item->iScale, c);
        // switch to alpha-blending to make the text visible over full screen-ish images
        if (item->alphaBlend)
            BeginBlendMode(BLEND_ALPHA);
        DrawTextEx(item->font, item->text.c_str(), item->tX + 2 * offset, item->tY, item->font.baseSize, 1.0, c);
    } 

    std::vector<CachedPosition> pos_;

    Transition t_ = Transition::None;
    Animation a_{500};
    int seamStart_;

    Texture defaultIcon_;

}; // BaseCarousel

/** Custom carousel that can be initialized with a hierarchical menu with closured for item selections. 
 */
class CCarousel : public BaseCarousel {
public:
    class BaseMenu {
    public:
        virtual ~BaseMenu() = default;
    protected:
        friend class CCarousel;

        std::string title;
        std::string icon;

        BaseMenu(std::string title, std::string icon):
            title{title}, icon{icon} {}
    }; 

    class Item : public BaseMenu {
    public:
        Item(std::string title, std::string icon, std::function<void()> onSelect):
            BaseMenu{title, icon},
            onSelect{onSelect} {
        }
    protected:
        friend class CCarousel;
        std::function<void()> onSelect;
    };

    class Menu : public BaseMenu {
    public:
        Menu(std::string title, std::string icon, std::initializer_list<BaseMenu *> subitems):
            BaseMenu{title, icon} {
            for (BaseMenu * bm : subitems)
            items.push_back(std::unique_ptr<BaseMenu>{bm});
        }
    protected:
        friend class CCarousel;
        std::vector<std::unique_ptr<BaseMenu>> items;
    }; 

    CCarousel(Menu * root):
        root_{root} {
        enter(root_.get());
    }

    void itemSelected(size_t index) override {
        BaseMenu * item = menu_.back()->items[index].get();
        Item * i = dynamic_cast<Item*>(item);
        if (i)
            i->onSelect();
        else 
            enter(dynamic_cast<Menu*>(item));
    }

    BaseCarousel::Item getItemFor(size_t index) override {
        BaseMenu * item = menu_.back()->items[index].get();
        if (item->icon.empty())
            return BaseCarousel::Item{item->title};
        else 
            return BaseCarousel::Item{item->title, item->icon};
    }

    void enter(Menu * menu) {
        menu_.push_back(menu);
        BaseCarousel::enter(menu->items.size());
    }

    void leave() override {
        menu_.pop_back();
        BaseCarousel::leave();
    }

private:

    std::vector<Menu*> menu_;
    std::unique_ptr<Menu> root_;
}; 


/** Carousel-style menu backed by a JSON object. 
 
    The JSON object must be an array of items, which are interpreted as items. Each item must be a JSON object that should contain the following:

    {
        "menu_title" : "Title text",
        "menu_icon" : "icon filename",
        "menu_subitems" : [], 
    }

    If the icon, or the title are missing a default value is displayed -- but missing on both is very likely an error. The item is either a leaf item, in which case the custom onItemSelected method of the carousel is called when the item is selected (button A), or can be a menu itself, which happens if 
 */
class JSONCarousel : public BaseCarousel {
public:
    static inline std::string const MENU_TITLE{"menu_title"};
    static inline std::string const MENU_ICON{"menu_icon"};
    static inline std::string const MENU_SUBITEMS{"menu_subitems"};

    JSONCarousel(json::Value root): root_{std::move(root)} {
        enter(& root_);
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
class CustomJSONCarousel : public JSONCarousel {
public:
    CustomJSONCarousel(json::Value json, std::function<void(json::Value const &)> onSelect):
        JSONCarousel{json},
        onSelect_{onSelect} {
    }

private:

    void itemSelected(size_t index, json::Value const & json) override { onSelect_(json); }

    std::function<void(json::Value const &)> onSelect_;
}; // CustomJSONCarousel

/** Carousel backed by a filesystem directory. 
 
    Show folders and files within. Useful for browsing the disk, can be used as a basic building block for file explorer. 
 */
class DirCarousel : public BaseCarousel {
public:

    DirCarousel(std::string const & root):
        root_{root} {
        enter(root);
    }

protected:

    using DirEntry = std::filesystem::directory_entry;
    using DirIterator = std::filesystem::directory_iterator;
    using Path = std::filesystem::path;

    struct DirInfo {
        Path dir;
        std::vector<DirEntry> entries;

        DirInfo(Path dir):
            dir{dir} {
            for (auto const & entry : DirIterator{dir})
                entries.push_back(entry);
        }

        DirInfo(DirInfo const &) = delete;
        DirInfo(DirInfo &&) = default;
    };

    void itemSelected(size_t index) override {
        DirEntry & e = dirs_.back().entries[index];
        if (e.is_directory()) {
            enter(e.path());
        }
    }

    Item getItemFor(size_t index) override {
        DirEntry & e = dirs_.back().entries[index];
        if (e.is_directory())
            return Item{"assets/images/014-info.png", e.path().stem()};
        else if (e.path().extension() == ".png") 
            return Item{e.path().stem(), e.path()};
        else
            return Item{e.path().stem()};
    }

    void enter(Path path) {
        dirs_.emplace_back(path);
        BaseCarousel::enter(dirs_.back().entries.size());
    }

    void leave() override {
        dirs_.pop_back();
        BaseCarousel::leave();
    }

    Path root_;

    std::vector<DirInfo> dirs_;

}; // DirCarousel
