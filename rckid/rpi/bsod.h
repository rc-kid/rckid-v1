#pragma once

#include "widget.h"
#include "window.h"

/** The blue screen of death:)
  
    Displays errors. Offers to restart the rckid app
 */
class BSOD : public Widget {
public:
    BSOD(Window * window, std::string const & title, std::string const & text):
        Widget{window},
        title_{title},
        text_{text} 
    {
    }

protected:

    void draw() override {

    }

    std::string title_;
    std::string text_;

}; // BSOD