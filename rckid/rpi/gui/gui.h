#pragma once

#include <cstdint>
#include <vector>
#include <string_view>
#include <string>

#include "raylib-cpp.h"

class GUI;

/** Basic Widget
 */
class Widget {
public:
    /** Override this to draw the widget. 
     */
    virtual void draw(GUI & gui) = 0;
}; // Widget

/** Basic class for the on-screen GUI elements used by RCKid. 
 
    # Header

    The header displays the status of RCKid.

    # Footer

    Footer usually displays only a help with what buttons do what action. 
    
 */
class GUI {
public:

    GUI();

    void setWidget(Widget * widget) {
        widget_ = widget;
    }

    void resetFooter() {
        footer_.clear();
    }

    void addFooterItem(Color color, std::string_view text) {
        footer_.push_back(FooterItem{color, std::string{text}});
    }

    void clearFooter() {
        footer_.clear();
    }

    void draw();

    int widgetWidth() const { return 320; }

    int widgetHeight() const { return 240; }

private:


    void drawHeader();
    void drawFooter();

    struct FooterItem {
        Color color;
        std::string text; 
    }; // GUI::FooterItem

    std::vector<FooterItem> footer_;

    Font helpFont_;
    Font menuFont_;
    Widget * widget_ = nullptr;

}; // GUI