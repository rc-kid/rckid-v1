#pragma once

#include <cstdint>
#include <vector>
#include <string_view>
#include <string>
#include <unordered_set>
#include <queue>
#include <mutex>
#include <condition_variable>


#include "raylib_cpp.h"
#include "events.h"
#include "menu.h"
#include "animation.h"
#include "widget.h"
#include "rckid.h"


static constexpr int Window_WIDTH = 320;
static constexpr int Window_HEIGHT = 240;
static constexpr int MENU_FONT_SIZE = 64;
static constexpr int HEADER_HEIGHT = 20;
static constexpr int FOOTER_HEIGHT = 20;

class Carousel;
class Window;

class FooterItem {
public:
    enum class Control {
        A, B, X, Y, L, R, Left,Right, Up, Down, Select, Start, Home, 
    };

    Control control() const { return control_; }
    std::string const & text() const { return text_; }

    FooterItem(Control control, std::string const & text):
        control_{control}, 
        text_{text} {
    }

    static FooterItem A(std::string const & text) { return FooterItem(Control::A, text); }
    static FooterItem B(std::string const & text) { return FooterItem(Control::B, text); }
    static FooterItem X(std::string const & text) { return FooterItem(Control::X, text); }
    static FooterItem Y(std::string const & text) { return FooterItem(Control::Y, text); }

private:

    friend class Window;

    int draw(Window * window, int x, int y) const ;

    Control control_;
    std::string text_;
    Vector2 textSize_;

}; // FooterItem


/** Basic class for the on-screen Window elements used by RCKid. 
 
    # Header

    The header displays the status of RCKid. It offers a simple and full mode, where the full mode is used when inside the home menu. The simple mode is intended for kids alone and consists of icons only, whereas the menu for adults contains also text information. 

    # Footer

    Footer usually displays only a help with what buttons do what action. 

 */
class Window {
public:

    Window();

    /** Sends given event to the UI main loop. 
     
        Can be called from any thread. 
    */
    void send(Event && ev) { events_.send(std::move(ev)); }

    comms::Mode mode() const { return mode_; }
    bool usb() const { return usb_; }
    bool charging() const { return charging_; }
    uint16_t vBatt() const { return vBatt_; }
    uint16_t vcc() const { return vcc_; }
    int16_t avrTemp() const { return avrTemp_; }
    int16_t accelTemp() const { return accelTemp_; }

    RCKid * rckid() { return rckid_; }

    void startRendering();

    void stopRendering();

    void loop(); 

    /** Shows the error in a modal window. 
     
        TODO
     */
    void error(std::string const & message) {
        TraceLog(LOG_ERROR, message.c_str());
    }

    void setWidget(Widget * widget);
    void setMenu(Menu * menu, size_t index = 0);
    void back(); 

    /** Returns the time increase since drawing of the last frame begun in milliseconds. Useful for advancing animations and generally keeping pace. 
     */    
    float redrawDelta() const { return redrawDelta_; }

    void resetFooter() {
        footer_.clear();
        if (nav_.size() > 0)
            addFooterItem(FooterItem::B("Back"));
    }

    void addFooterItem(FooterItem item) {
        item.textSize_ = MeasureTextEx(helpFont_, item.text_.c_str(), 16, 1.0);
        footer_.push_back(item);
    }

    void clearFooter() {
        footer_.clear();
    }

    Font const & menuFont() const { return menuFont_; }
    Font const & helpFont() const { return helpFont_; }

private:

    friend class WindowElement;
    friend class Widget;

    class NavigationItem {
    public:
        enum class Kind {
            Menu, 
            Widget
        }; 

        Kind kind;
        
        Widget * widget() const {
            ASSERT(kind == Kind::Widget);
            return widget_;
        }

        Menu * menu() const {
            ASSERT(kind == Kind::Menu);
            return menu_;
        }

        size_t menuIndex() const {
            ASSERT(kind == Kind::Menu);
            return menuIndex_;
        }

        NavigationItem(): kind{Kind::Widget}, widget_{nullptr} {}

        NavigationItem(Widget * widget):
            kind{Kind::Widget},
            widget_{widget} {
        }

        NavigationItem(Menu * menu, size_t index):
            kind{Kind::Menu},
            menu_{menu},
            menuIndex_{index} {
        }

    private:

        union {
            Widget * widget_;
            Menu * menu_;
        };

        size_t menuIndex_ = 0;
    }; 

    void attach(WindowElement * element) {
        elements_.insert(element);
    }

    void detach(WindowElement * element) {
        elements_.erase(element);
    }

    void draw();

    void btnA(bool state) { if (widget_ && ! swap_.running()) widget_->btnA(state); }
    void btnB(bool state) { 
        if (widget_)
            widget_->btnB(state);
        if (state && ! swap_.running())
            back();
    }
    void btnX(bool state) { if (widget_) widget_->btnX(state); }
    void btnY(bool state) { if (widget_) widget_->btnY(state); }
    void btnL(bool state) { if (widget_) widget_->btnL(state); }
    void btnR(bool state) { if (widget_) widget_->btnR(state); }
    void btnSelect(bool state) { if (widget_) widget_->btnSelect(state); }
    void btnStart(bool state) { if (widget_) widget_->btnStart(state); }
    void btnJoy(bool state) { if (widget_) widget_->btnJoy(state); }
    void dpadLeft(bool state) { if (widget_) widget_->dpadLeft(state); }
    void dpadRight(bool state) { if (widget_) widget_->dpadRight(state); }
    void dpadUp(bool state) { if (widget_) widget_->dpadUp(state); }
    void dpadDown(bool state) { if (widget_) widget_->dpadDown(state); }
    void joy(uint8_t x, uint8_t y) { if (widget_) widget_->joy(x, y); }
    void accel(uint8_t x, uint8_t y) { if (widget_) widget_->accel(x, y); }

    void btnVolUp(bool state) { if (widget_) widget_->btnVolUp(state); }

    void btnVolDown(bool state) { if (widget_) widget_->btnVolDown(state); }

    void btnHome(bool state) { 
        if (widget_) 
            widget_->btnHome(state); 
        if (state && ! swap_.running())
            setMenu(homeMenu_, 0); 
    }

    void swapWidget();

    void drawHeader();
    void drawFooter();

    // the rckid driver
    RCKid * rckid_; 

    EventQueue<Event> events_;

    bool rendering_ = false;


    std::unordered_set<WindowElement*> elements_;
    std::vector<FooterItem> footer_;

    double lastDrawTime_;
    float redrawDelta_;
    Font helpFont_;
    Font headerFont_;
    Font menuFont_;
    Widget * widget_ = nullptr;
    NavigationItem next_;
    /// the carousel used for the menus 
    Carousel * carousel_; 
    Menu * homeMenu_;

    std::vector<NavigationItem> nav_;
    bool inHomeMenu_ = false;

    enum class Transition {
        FadeIn, 
        FadeOut, 
        None, 
    }; 

    Animation swap_{250};
    Transition transition_ = Transition::None;

    /** RCKid status
     */
    comms::Mode mode_;
    bool usb_;
    bool charging_;
    uint16_t vBatt_ = 420;
    uint16_t vcc_ = 430;
    int16_t avrTemp_;
    int16_t accelTemp_;

    static constexpr int GLYPHS[] = {
        32, 33, 34, 35, 36,37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, // space & various punctuations
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57, // 0..9
        58, 59, 60, 61, 62, 63, 64, // more punctuations
        65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, // A-Z
        91, 92, 93, 94, 95, 96, // more punctuations
        97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, // a-z
        123, 124, 125, 126, // more punctiations
        0xf004, 0xf08a, //  
        0xf05a9, 0xf05aa, 0xf05aa, 0xf16c1, // 󰖩 󰖪 󰖪 󱛁
        0xf244, 0xf243, 0xf242, 0xf241, 0xf240, 0xf0e7, //      
        0xf02cb, // 󰋋
        0xf1119, // 󱄙
        0xf0e08, 0xf057f, 0xf0580, 0xf057e, // 󰸈 󰕿 󰖀 󰕾
    };

}; // Window