#pragma once

class Window;
class RCKid;

/** Basic Widget
 */
class Widget {
public:
protected:

    friend class RCKid;

    Widget(Window * window): window_{window} { };

    /** Returns true if the widget is fullscreen. Fullscreen widgets will not have the footer drawn in the bottom of the screen, which saves some space. 
     */
    virtual bool fullscreen() const { return false; }

    /** Override this to draw the widget.
     */
    virtual void draw() = 0;

    /** Called when the widget does not need redraw, but frame event occurs. 
     */
    virtual void idle() {}

    /** Called when the widget gains focus (becomes visible). 
     */
    virtual void onFocus() {};

    /** Called when the widget loses focus (stops being displayed). 
    */
    virtual void onBlur() {};

    /** Called when the widget enters the navigation stack. 
    */
    virtual void onNavigationPush() {};

    /** Called when the widget leaves the navigation stack. 
     */
    virtual void onNavigationPop() {};
    
    virtual void btnA(bool state) {}

    virtual void btnB(bool state);

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
    virtual void btnHome(bool state);

    /** Called when audio packet (32 bytes) has been recorded by the AVR. 
     */
    virtual void audioRecorded(RecordingEvent & e) {}

    /** Packet received callback. 
     */
    virtual void nrfPacketReceived(uint8_t  * packet) {}
    
    /** Packet transmit callback. 
     */
    virtual void nrfTxCallback(bool ok) {}





    Window * window() { return window_; }

    Window * window_;


protected:

    void requestRedraw() { redraw_ = true; }
    void cancelRedraw() { redraw_ = false; }
private: 

    friend class Window;

    /** True if the widget is currently on the navigation stack. 
     */
    bool onNavStack_ = false;
    bool redraw_ = true;


}; // Widget
