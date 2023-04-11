#pragma once

class GUI;

/** Basic Widget
 */
class Widget {
public:
protected:

    friend class GUI;

    Widget() = default;

    /** Returns true if the widget is fullscreen. Fullscreen widgets will not have the footer drawn in the bottom of the screen, which saves some space. 
     */
    virtual bool fullscreen() const { return false; }

    /** Override this to draw the widget.
     */
    virtual void draw(GUI * gui) = 0;

    /** Called when the widget gains focus (becomes visible). 
     */
    virtual void onFocus(GUI * gui) {};

    /** Called when the widget loses focus (stops being displayed). 
    */
    virtual void onBlur(GUI * gui) {};
    
    virtual void btnA(GUI * gui, bool state) {}
    virtual void btnB(GUI * gui, bool state) {}
    virtual void btnX(GUI * gui, bool state) {}
    virtual void btnY(GUI * gui, bool state) {}
    virtual void btnL(GUI * gui, bool state) {}
    virtual void btnR(GUI * gui, bool state) {}
    virtual void btnSelect(GUI * gui, bool state) {}
    virtual void btnStart(GUI * gui, bool state) {}
    virtual void btnJoy(GUI * gui, bool state) {}
    virtual void dpadLeft(GUI * gui, bool state) {}
    virtual void dpadRight(GUI * gui, bool state) {}
    virtual void dpadUp(GUI * gui, bool state) {}
    virtual void dpadDown(GUI * gui, bool state) {}
    virtual void joy(GUI * gui, uint8_t x, uint8_t y) {}
    virtual void accel(GUI * gui, uint8_t x, uint8_t y) {}
    virtual void btnVolUp(GUI * gui, bool state) {}
    virtual void btnVolDown(GUI * gui, bool state) {}
    virtual void btnHome(GUI * gui, bool state) {}

}; // Widget
