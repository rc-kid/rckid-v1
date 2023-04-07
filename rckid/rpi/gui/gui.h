#pragma once

#include <cstdint>
#include <vector>
#include <string_view>
#include <string>

#include "raylib_cpp.h"
#include "menu.h"
#include "widget_helper.h"

class GUI;
class Carousel;

static constexpr int GUI_WIDTH = 320;
static constexpr int GUI_HEIGHT = 240;
static constexpr int MENU_FONT_SIZE = 64;
static constexpr int HEADER_HEIGHT = 20;
static constexpr int FOOTER_HEIGHT = 20;

/** Basic Widget
 */
class Widget : public WidgetHelper {
public:

protected:

    friend class GUI;

    Widget(GUI * gui):
        gui_{gui} {
    }

    GUI * gui() const { return gui_; }



    /** Override this to draw the widget.
     */
    virtual void draw(double deltaMs) = 0;

    virtual void btnA(bool state) {}
    virtual void btnB(bool state);
    virtual void btnX(bool state) {}
    virtual void btnY(bool state) {}
    virtual void btnL(bool state) {}
    virtual void btnR(bool state) {}
    virtual void btnSelect(bool state) {}
    virtual void btnStart(bool state) {}
    virtual void btnDpad(bool state) {}
    virtual void dpadLeft(bool state) {}
    virtual void dpadRight(bool state) {}
    virtual void dpadUp(bool state) {}
    virtual void dpadDown(bool state) {}
    virtual void joy(uint8_t x, uint8_t y) {}
    virtual void accel(uint8_t x, uint8_t y) {}
    virtual void btnVolUp(bool state) {}
    virtual void btnVolDown(bool state) {}
    virtual void btnHome(bool state);

private:

    GUI * gui_;
}; // Widget

/** Basic class for the on-screen GUI elements used by RCKid. 
 
    # Header

    The header displays the status of RCKid. It offers a simple and full mode, where the full mode is used when inside the home menu. The simple mode is intended for kids alone and consists of icons only, whereas the menu for adults contains also text information. 

    # Footer

    Footer usually displays only a help with what buttons do what action. 

 */
class GUI {
public:

    GUI();

    void setWidget(Widget * widget);
    void setMenu(Menu * menu, size_t index = 0);
    void back(); 

    void resetFooter() {
        footer_.clear();
    }

    void addFooterItem(Color color, std::string_view text) {
        footer_.push_back(FooterItem{color, std::string{text}});
    }

    void clearFooter() {
        footer_.clear();
    }

    void processInputEvents();

    void draw();

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

        Kind const kind;
        
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

    void btnA(bool state) { if (widget_) widget_->btnA(state); }
    void btnB(bool state) { if (widget_) widget_->btnB(state); }
    void btnX(bool state) { if (widget_) widget_->btnX(state); }
    void btnY(bool state) { if (widget_) widget_->btnY(state); }
    void btnL(bool state) { if (widget_) widget_->btnL(state); }
    void btnR(bool state) { if (widget_) widget_->btnR(state); }
    void btnSelect(bool state) { if (widget_) widget_->btnSelect(state); }
    void btnStart(bool state) { if (widget_) widget_->btnStart(state); }
    void btnDpad(bool state) { if (widget_) widget_->btnDpad(state); }
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
        else 
             setMenu(homeMenu_, 0); 
    }

    void drawHeader();
    void drawFooter();

    struct FooterItem {
        Color color;
        std::string text; 
    }; // GUI::FooterItem

    std::vector<FooterItem> footer_;

    double lastDrawTime_;
    Font helpFont_;
    Font menuFont_;
    Widget * widget_ = nullptr;
    /// the carousel used for the menus 
    Carousel * carousel_; 
    Menu * homeMenu_;

    std::vector<NavigationItem> nav_;

}; // GUI