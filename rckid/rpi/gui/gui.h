#pragma once

#include <cstdint>
#include <vector>
#include <string_view>
#include <string>

#include "raylib_cpp.h"
#include "menu.h"
#include "animation.h"
#include "widget.h"
#include "../rckid.h"


static constexpr int GUI_WIDTH = 320;
static constexpr int GUI_HEIGHT = 240;
static constexpr int MENU_FONT_SIZE = 64;
static constexpr int HEADER_HEIGHT = 20;
static constexpr int FOOTER_HEIGHT = 20;

class Carousel;
class GUI;

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

    friend class GUI;

    int draw(GUI * gui, int x, int y) const ;

    Control control_;
    std::string text_;
    Vector2 textSize_;

}; // FooterItem


/** Basic class for the on-screen GUI elements used by RCKid. 
 
    # Header

    The header displays the status of RCKid. It offers a simple and full mode, where the full mode is used when inside the home menu. The simple mode is intended for kids alone and consists of icons only, whereas the menu for adults contains also text information. 

    # Footer

    Footer usually displays only a help with what buttons do what action. 

 */
class GUI {
public:

    GUI();

    void loop(RCKid * driver); 

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

    void processInputEvents(RCKid * rckid);

    void draw();


    void btnA(bool state) { if (widget_ && ! swap_.running()) widget_->btnA(this, state); }
    void btnB(bool state) { 
        if (widget_)
            widget_->btnB(this, state);
        if (state && ! swap_.running())
            back();
    }
    void btnX(bool state) { if (widget_) widget_->btnX(this, state); }
    void btnY(bool state) { if (widget_) widget_->btnY(this, state); }
    void btnL(bool state) { if (widget_) widget_->btnL(this, state); }
    void btnR(bool state) { if (widget_) widget_->btnR(this, state); }
    void btnSelect(bool state) { if (widget_) widget_->btnSelect(this, state); }
    void btnStart(bool state) { if (widget_) widget_->btnStart(this, state); }
    void btnJoy(bool state) { if (widget_) widget_->btnJoy(this, state); }
    void dpadLeft(bool state) { if (widget_) widget_->dpadLeft(this, state); }
    void dpadRight(bool state) { if (widget_) widget_->dpadRight(this, state); }
    void dpadUp(bool state) { if (widget_) widget_->dpadUp(this, state); }
    void dpadDown(bool state) { if (widget_) widget_->dpadDown(this, state); }
    void joy(uint8_t x, uint8_t y) { if (widget_) widget_->joy(this, x, y); }
    void accel(uint8_t x, uint8_t y) { if (widget_) widget_->accel(this, x, y); }

    void btnVolUp(bool state) { if (widget_) widget_->btnVolUp(this, state); }

    void btnVolDown(bool state) { if (widget_) widget_->btnVolDown(this, state); }

    void btnHome(bool state) { 
        if (widget_) 
            widget_->btnHome(this, state); 
        if (state && ! swap_.running())
            setMenu(homeMenu_, 0); 
    }


    void swapWidget();

    void drawHeader();
    void drawFooter();

    std::vector<FooterItem> footer_;

    double lastDrawTime_;
    float redrawDelta_;
    Font helpFont_;
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

}; // GUI