#pragma once

#include <vector>
#include <functional>
#include <memory>

#include "utils/json.h"

#include "widget.h"
#include "window.h"
#include "animation.h"

/** Basic carousel functionality and features common for various carousel types. 

    - does the item rendering, and others
    - keeps the  
 
*/
class BaseCarousel : public Widget {
protected:

    BaseCarousel():
        defaultIcon_{"assets/images/008-unicorn.png"} {
    }

    BaseCarousel(std::string const & defaultIcon):
        defaultIcon_{defaultIcon} {
    }

    /** Item information consisting of the image and text to display with the item. 
     */
    class Item {
    public:
        Item(std::string const & text, Canvas::Texture icon):
            icon{icon}, text{text} {
        }

        Item(std::string const & text, std::string const & iconFile):
            icon{iconFile},
            text{text} {
        }

        Item(std::string const & text): text{text} {}

        Item(Item const &) = delete;

        Item(Item && from): icon{from.icon}, text{std::move(from.text)} {}

        bool useDefaultIcon() const { return !icon.valid(); }

        Canvas::Texture icon;
        std::string text;

    }; 

    /** Resets the carousel. 
     
        The carousel reset should clear all cached information and reset the position to be root of the carousel. 
     */
    virtual void reset() {
        pos_.clear();
    }

    size_t currentIndex() { return pos().current; }

    size_t currentNumItems() { return pos().numItems; }

    std::string const & currentTitle() { return getItem(pos().current)->text; }

    virtual void goToPrev() {
        pos().current = pos().prev();
        startTransition(Transition::Prev);
    }

    virtual void goToNext() {
        pos().current = pos().next();
        startTransition(Transition::Next);
    }

    /** Determines whether the carousel will also show the item's title, or only the item's icon. 
    */
    //@{
    bool showTitle() const { return showTitle_; }
    void setShowTitle(bool value = true) { showTitle_ = value; requestRedraw(); }
    //@}

    /** Determines whether the carousel will keep moving left/right when the respective action button will be pressed while the transition ends. 
     */
    //@{
    bool autorepeat() const { return autorepeat_; }
    void setAutorepeat(bool value = true) { autorepeat_ = true; }
    //@}

protected:

    bool busy() const { return a_.running(); }

    /** Creates the information about the index-th item. 
     
        The item consists of the icon and the title and must be implemented in the data providing subclasses. 
     */
    virtual Item getItemFor(size_t index) = 0;

    /** What to do when an item is selected. 
     */
    virtual void itemSelected(size_t index) {}

    virtual void enter(size_t numItems) {
        pos_.emplace_back(numItems);
        startTransition(pos_.size() == 1 ? Transition::FadeIn : Transition::EnterFadeOut);
    }

    virtual void leave() {
        ASSERT(pos_.size() > 1);
        startTransition(Transition::LeaveFadeOut);
    }

    /** Reset the carousel when pushed on the navigation stack to ensure we start from valid and consistent state. If the carousel should keep its position between invocations, the reset can be disabled, but a reset call *must* be issued before drawing to ensure the carousel is in valid state (!!)
     */
    void onNavigationPush() override { 
        reset(); 
    }

    void setFooterHints() override {
        // don't call the Widget's set footer as we do the reset ourselves
        window().resetFooter(/* forceBack */ pos_.size() > 1);
        window().addFooterItem(FooterItem::A("Select"));
    }

    void draw(Canvas & canvas) override {
        a_.update();
        if (pos().numItems == 0) {
            drawEmpty();
            return;
        }
        switch (t_) {
            case Transition::None:
                cancelRedraw();
                drawItem(canvas, getItem(pos().current), 0, 255);
                break;
            // when the transition is to the left item, the current item is on the left and we are moving right, i.e. current item's offset moves from -320 to 0 and the rightOf (previously current) moves from 0 to 320. The animation moves to the *right*
            case Transition::Prev: {
                int offset = a_.interpolate(0, Window_WIDTH);
                window().setBackgroundSeam(seamStart_ + offset / 4);
                drawItem(canvas, getItem(pos().next()), offset, 255);
                drawItem(canvas, getItem(pos().current), offset - Window_WIDTH, 255);   
                break;
            }
            // for right transition, the current item is on the right and we move to the left, i.e. the leftOf item starts at 0 and goes to -320, while the current item starts at 320 and goes to 0
            case Transition::Next: {
                int offset = a_.interpolate(0, Window_WIDTH);
                window().setBackgroundSeam(seamStart_ - offset / 4);
                drawItem(canvas, getItem(pos().prev()), - offset, 255);
                drawItem(canvas, getItem(pos().current), Window_WIDTH - offset, 255);                
                break;
            }
            case Transition::EnterFadeOut: {
                int alpha = a_.interpolate(255, 0);
                drawItem(canvas, posPrev().items[posPrev().current].get(), 0, alpha);
                if (a_.done())
                    startTransition(Transition::FadeIn);
                break;
            }
            case Transition::LeaveFadeOut: {
                int alpha = a_.interpolate(255, 0);
                drawItem(canvas, pos().items[pos().current].get(), 0, alpha);
                if (a_.done()) {
                    pos_.pop_back();
                    startTransition(Transition::FadeIn);
                }
                break;
            }
            case Transition::FadeIn: {
                int alpha = a_.interpolate(0, 255);
                drawItem(canvas, getItem(pos().current), 0, alpha);
                break;
            }
        }
        if (a_.done()) {
            t_ = Transition::None;
            if (autorepeat_) { 
                if (tRepeat_ == Transition::Prev)
                    goToPrev();
                else if (tRepeat_ == Transition::Next)
                    goToNext();
            }             
        }
    }

    virtual void drawEmpty() {
        // TODO do something useful
    }

    void btnA(bool state) override {
        if (state && !busy())
            itemSelected(pos().current);
    }

    void btnB(bool state) override {
        if (state && !busy() && pos_.size() > 1)
            leave();
        else
            Widget::btnB(state);
    }

    void dpadLeft(bool state) override {
        if (state) {
            if (!busy()) {
                goToPrev();
                tRepeat_ = Transition::Prev;
            }
        } else {
            tRepeat_ = Transition::None;
        }
    }

    void dpadRight(bool state) override {
        if (state) {
            if (!busy()) {
                goToNext();
                tRepeat_ = Transition::Next;
            }
        } else {
            tRepeat_ = Transition::None;
        }
    }

    /** By default the carousel allows controlling volume using the up/down dpad keys. This behaviour can be overriden in children when appropriate. 
     */
    void dpadUp(bool state) override {
        if (state)
            rckid().setVolume(rckid().volume() + 10);
    }

    void dpadDown(bool state) override {
        if (state)
            rckid().setVolume(rckid().volume() - 10);
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
                iY = 24 + 5;
            } else {
                if (icon.height() <= 128) {
                    iY = 88 - icon.height() / 2 + 5;
                } else  {
                    // the text might cover some of the icon, set alpha blending instead of the default additive blending that would be used over transparent background
                    alphaBlend = true;
                    if (icon.height() > 240)
                        iScale = 240.f / icon.height();
                    else
                        iY = (240 - icon.height()) / 2;
                }
                if (icon.width() <= 320) {
                    iX = (320 - icon.width()) / 2;
                } else {
                    float s = 320.f / icon.width();
                    if (s < iScale)
                        iScale = s;
                }    
                if (iScale != 1.0) {
                    iY = static_cast<int>((240 - icon.height() * iScale) / 2);
                    iX = static_cast<int>((320 - icon.width() * iScale) / 2);
                }
            }
            textWidth = window().canvas().textWidth(text, window().canvas().titleFont());;
            tY = 157;
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
        if (focused()) {
            requestRedraw();
            t_ = t;
            switch (t) {
                case Transition::Prev:
                case Transition::Next:
                    seamStart_ = window().backgroundSeam();
                    a_.start(WIDGET_FADE_TIME * 2);
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
            a_.start(WIDGET_FADE_TIME);
        }
    }

    /** Draws the previously created item
     */
    void drawItem(Canvas & c, CachedItem * item, int offset, uint8_t alpha) {
        if (item == nullptr)
            return;
        Color color{RGBA(255, 255, 255, alpha)};
        if (item->iScale == 1.0)
            c.drawTexture(item->iX + offset, item->iY, item->useDefaultIcon() ? defaultIcon_ : item->icon, color);    
        else
            c.drawTextureScaled(item->iX + offset, item->iY, item->icon, item->iScale, color);    
        // switch to alpha-blending to make the text visible over full screen-ish images
        if (showTitle_) {
            if (item->alphaBlend)
                BeginBlendMode(BLEND_ALPHA);
            if (c.drawScrolledText(offset, item->tY, 320, item->text, item->textWidth, color, c.titleFont()))
                requestRedraw();
        }
    } 

    std::vector<CachedPosition> pos_;

    Transition t_ = Transition::None;
    Animation a_{500};
    int seamStart_;

    bool showTitle_ = true;
    bool autorepeat_ = true;
    Transition tRepeat_ = Transition::None;

    Canvas::Texture defaultIcon_;

}; // BaseCarousel

/** Custom carousel that can be initialized with a hierarchical menu with closured for item selections. 
 */
class Carousel : public BaseCarousel {
public:
    class BaseMenu {
    public:
        virtual ~BaseMenu() = default;
    protected:
        friend class Carousel;

        std::string title;
        std::string icon;

        BaseMenu(std::string title, std::string icon):
            title{title}, icon{icon} {}

        BaseMenu() = default;
    }; 

    class Item : public BaseMenu {
    public:
        Item(std::string title, std::string icon, std::function<void()> onSelect):
            BaseMenu{title, icon},
            onSelect{onSelect} {
        }
    protected:
        friend class Carousel;
        std::function<void()> onSelect;
    };

    class Menu : public BaseMenu {
    public:
        Menu(std::string title, std::string icon, std::initializer_list<BaseMenu *> subitems):
            BaseMenu{title, icon} {
            for (BaseMenu * bm : subitems)
              items_.push_back(std::unique_ptr<BaseMenu>{bm});
        }

        Menu() = default;

        void append(BaseMenu * m) {
            items_.push_back(std::unique_ptr<BaseMenu>{m});
        }

        size_t size() const { return items_.size(); }

        bool empty() const { return items_.empty(); }

    protected:
        friend class Carousel;
        std::vector<std::unique_ptr<BaseMenu>> items_;
    }; 

    Carousel(Menu * root):
        root_{root} {
    }

    void reset() override {
        BaseCarousel::reset();
        enter(root_.get());
    }

    void itemSelected(size_t index) override {
        BaseMenu * item = menu_.back()->items_[index].get();
        Item * i = dynamic_cast<Item*>(item);
        if (i)
            i->onSelect();
        else 
            enter(dynamic_cast<Menu*>(item));
    }

    BaseCarousel::Item getItemFor(size_t index) override {
        BaseMenu * item = menu_.back()->items_[index].get();
        if (item->icon.empty())
            return BaseCarousel::Item{item->title};
        else 
            return BaseCarousel::Item{item->title, item->icon};
    }

    void enter(Menu * menu) {
        menu_.push_back(menu);
        BaseCarousel::enter(menu->items_.size());
    }

    void leave() override {
        menu_.pop_back();
        BaseCarousel::leave();
    }

private:

    std::vector<Menu*> menu_;
    std::unique_ptr<Menu> root_;
}; 
