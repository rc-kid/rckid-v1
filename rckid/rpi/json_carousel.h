#pragma once

#include "utils/json.h"

#include "widget.h"
#include "animation.h"

/** Basic carousel functionality and features common for various carousel types. 

    - does the item rendering, and others
    - keeps the  
 
*/
class BaseCarousel : public Widget {
protected:

    /** Item information consisting of the image and text to display with the item. 
     
        Note that the 
     */
    struct Item {
        Texture2D icon;
        std::string text;
    }; 

    struct Position {
        size_t numItems;
        size_t current;
    }; 

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

    /** Draws the previously created item
     */
    void drawItem(Item * item, int offset, uint8_t alpha) {
        if (item == nullptr)
            return;
        Color c{RGBA(255, 255, 255, alpha)};
        DrawTexture(item->icon, item->iX + offset, item->iY, c);
        DrawTextEx(item->font, item->text, item->ix + 2 * offset, c);
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

/** Carousel-style menu backed by a JSON object. 
 
    The JSON object must be an array of items, which are interpreted as items. Each item must be a JSON object that should contain the following:

    {
        "title" : "Title text",
        "icon" : "icon filename"
    }

    If the icon, or the title are missing a default value is displayed -- but missing on both is very likely an error. The item is either a leaf item, in which case the custom onItemSelected method of the carousel is called when the item is selected (button A), or can be a menu itself, which happens if 

    

 */
class JSONCarousel : public Widget {
public:



protected:

    /** Called when a leaf item (non-array) is selected */
    virtual void onItemSelected(json::Value & value) {}

    json::Value json_;
    json::Value * current_ = nullptr;
    std::vector<size_t> position;
}; 