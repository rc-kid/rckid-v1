#pragma once

#include "raylib_cpp.h"
#include "events.h"

class Window;
class RCKid;

/** Basic Widget
 */
class Widget {
public:
    bool focused() const;
    bool onNavStack() const { return onNavStack_; }
protected:

    friend class RCKid;

    /** Returns true if the widget is fullscreen. Fullscreen widgets will not have the footer drawn in the bottom of the screen, which saves some space. 
     */
    virtual bool fullscreen() const { return false; }

    /** Called every time the UI is about to redraw itself (approx 60times per second in ideal conditions)
     */
    virtual void tick() {};

    /** Override this to draw the widget.
     */
    virtual void draw() = 0;

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
    virtual void joy(uint8_t h, uint8_t v) {}
    virtual void accel(uint8_t h, uint8_t v) {}
    virtual void btnHome(bool state);

    /** Called when audio packet (32 bytes) has been recorded by the AVR. 
     */
    virtual void audioRecorded(RecordingEvent & e) {}

    /** Packet received callback. 
     */
    virtual void nrfPacketReceived(NRFPacketEvent & e) {}
    
    /** Packet transmit callback. 
     */
    virtual void nrfTxDone() {}


    /** Updates the widget's footer shortcut information. 
     
        Called automatically by the framework when the widget is focused, but can also be called programatically if the information changes. 
     */
    virtual void setFooterHints();

    void requestRedraw() { redraw_ = true; }
    void cancelRedraw() { redraw_ = false; }

private: 

    friend class Window;

    /** True if the widget is currently on the navigation stack. 
     */
    bool onNavStack_ = false;
    bool redraw_ = true;

}; // Widget
