#pragma once

#include <filesystem>
#include <vector>

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
        Item(Texture icon, std::string const & text):
            icon{icon}, text{text} {
        }

        Item(std::string const & iconFile, std::string const & text):
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
        if (pos_.size() > 1)
            startTransition(Transition::EnterFadeOut);
    }

    virtual void leave() {
        if (pos_.size() > 1) {
            startTransition(Transition::LeaveFadeOut);
        } else {
            // TODO window's nav pane go back
        }
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
        if (state)
            itemSelected(pos().current);
    }

    void btnB(bool state) override {
        if (state)
            leave();
    }

    void dpadLeft(bool state) override {
        if (state)
            goToPrev();
    }

    void dpadRight(bool state) override {
        if (state)
            goToNext();
    }



/*
    void setPosition(size_t index) {
        ASSERT(index < pos().numItems);
        pos().current = index;
    }

    void resetPosition(size_t numItems) {
        if (pos_.empty())
            pos_.emplace_back(numItems);
        else
            pos_.back().reset(numItems);
    }
*/

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

        CachedItem(Item && item):
            Item{std::move(item)} {
            if (useDefaultIcon()) {
                iX = 96;
                iY = 24;
            } else {
                if (icon.height <= 128) {
                    iY = 88 - icon.height / 2;
                } else  {
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

        /*
        void reset(size_t numItems) {
            this->numItems = numItems;
            current = 0;
            items.clear();
            itemr.resize(numItems);
        } */
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
        a_.start();
        t_ = t;
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
        DrawTextEx(item->font, item->text.c_str(), item->tX + 2 * offset, item->tY, item->font.baseSize, 1.0, c);
    } 

    std::vector<CachedPosition> pos_;

    Transition t_ = Transition::None;
    Animation a_{500};
    int seamStart_;

    Texture defaultIcon_;


}; // BaseCarousel

/** Carousel backed by a filesystem directory. 
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
            return Item{e.path(), e.path().stem()};
        else
            return Item{dirs_.back().entries[index].path().stem()};
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

}; // FolderCarousel



#ifdef FOO

    void draw() override {
        if (a_.update()) 
            t_ = Transition::None;
        switch (t_) {
            case Transition::None:
                drawItem(cachedItems[i_], 0, 255);
                cancelRedraw();
                break;
            // when the transition is to the left item, the current item is on the left and we are moving right, i.e. current item's offset moves from -320 to 0 and the rightOf (previously current) moves from 0 to 320. The animation moves to the *right*
            case Transition::Left: {
                int offset = a_.interpolate(0, Window_WIDTH);
                window().setBackgroundSeam(seamStart_ + offset / 4);
                drawItem(item(rightOf()), offset, 255);
                drawItem(item(i_), offset - Window_WIDTH, 255);                
                break;
            }
            // for right transition, the current item is on the right and we move to the left, i.e. the leftOf item starts at 0 and goes to -320, while the current item starts at 320 and goes to 0
            case Transition::Right: {
                int offset = a_.interpolate(0, Window_WIDTH);
                window().setBackgroundSeam(seamStart_ - offset / 4);
                drawItem(item(leftOf()), - offset, 255);
                drawItem(item(i_), Window_WIDTH - offset, 255);                
                break;
            }
            // TODO in & out
        }
    }

    /** Gets the index-th item information, guaranteed to be executed only once per item and then cached for as long as the item's location stays in the navigation stack. 
     */
    virtual Item getItemFor(size_t index) = 0;

    virtual void back() { }

    virtual Position next() = 0;

private:

    size_t current() { return pos_.back(); }
    size_t leftOf(size_t i) { return i == 0 ? (numItems_ - 1) : i_ - 1; }
    size_t rightOf(size_t i) { return i == numItems_ - 1 ? 0 : i_ + 1; }
    
    Item * item(size_t i) { 
        if (cachedItems_[i] == nullptr)
            cachedItems_[i] = new CachedItem{getItemFor(i)};
        return cachedItems_[i];
    }


    /** Currently animated transition. 
     */
    enum class Transition {
        Left, 
        Right,
        In, 
        Out,  
        None, 
    }; 


    /** Cached wraps the user returned item with extra bookkeeping for speedy rendering. 
     */
    struct CachedItem: Item {
        int iX;
        int iY;

        Font font;
        int textWidth;
        int tX;
        int tY;

        CachedItem(Item && item):
            Item{std::move(item)} {
            
        }
    }; 

    struct CachedPosition : Position {
        std::vector<Item *> cachedItems_;
    }; 

    std::vector<CachedPosition> position_;

    Transition t_ = Transition::None;
    Animation a_;
}; // BaseCarousel



#endif

/** Carousel-style menu backed by a JSON object. 
 
    The JSON object must be an array of items, which are interpreted as items. Each item must be a JSON object that should contain the following:

    {
        "title" : "Title text",
        "icon" : "icon filename"
    }

    If the icon, or the title are missing a default value is displayed -- but missing on both is very likely an error. The item is either a leaf item, in which case the custom onItemSelected method of the carousel is called when the item is selected (button A), or can be a menu itself, which happens if 

    

 */
#ifdef FOO
class JSONCarousel : public Widget {
public:



protected:

    /** Called when a leaf item (non-array) is selected */
    virtual void onItemSelected(json::Value & value) {}

    json::Value json_;
    json::Value * current_ = nullptr;
    std::vector<size_t> position;
}; 



#endif