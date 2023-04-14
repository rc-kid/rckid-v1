#pragma once

class GUI;

class GUIElement {
public:

    virtual ~GUIElement();

protected:

    friend class GUI;

    GUIElement(GUI * gui);

    virtual void onRenderingPaused() {}

    GUI * gui() { return gui_; }

    GUI * gui_;
}; 

/** Basic Widget
 */
class Widget : public GUIElement {
public:
protected:

    friend class GUI;

    Widget(GUI * gui): GUIElement{gui} { };

    /** Returns true if the widget is fullscreen. Fullscreen widgets will not have the footer drawn in the bottom of the screen, which saves some space. 
     */
    virtual bool fullscreen() const { return false; }

    /** Override this to draw the widget.
     */
    virtual void draw() = 0;

    /** Called when the widget gains focus (becomes visible). 
     */
    virtual void onFocus() {};

    /** Called when the widget loses focus (stops being displayed). 
    */
    virtual void onBlur() {};
    
    virtual void btnA(bool state) {}
    virtual void btnB(bool state) {}
    virtual void btnX(bool state) {}
    virtual void btnY(bool state) {}
    virtual void btnL(bool state) {}
    virtual void btnR(bool state) {}
    virtual void btnSelect(bool state) {}
    virtual void btnStart(bool state) {}
    virtual void btnJoy(bool state) {}
    virtual void dpadLeft(bool state) {}
    virtual void dpadRight(bool state) {}
    virtual void dpadUp(bool state) {}
    virtual void dpadDown(bool state) {}
    virtual void joy(uint8_t x, uint8_t y) {}
    virtual void accel(uint8_t x, uint8_t y) {}
    virtual void btnVolUp(bool state) {}
    virtual void btnVolDown(bool state) {}
    virtual void btnHome(bool state) {}

}; // Widget
