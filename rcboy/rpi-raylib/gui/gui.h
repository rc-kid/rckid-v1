#pragma once

#include <cstdint>
#include <vector>
#include <string_view>
#include <string>

#include "raylib-cpp.h"

/** Basic class for the on-screen GUI elements used by RCboy. 
 
    # Header

    The header displays the status of RCBoy.

    # Footer

    Footer usually displays only a help with what buttons do what action. 
    
 */
class GUI {
public:

    GUI();

    void resetFooter() {
        footer_.clear();
    }

    void addFooterItem(Color color, std::string_view text) {
        footer_.push_back(FooterItem{color, std::string{text}});
    }

    void draw();

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

}; // GUI