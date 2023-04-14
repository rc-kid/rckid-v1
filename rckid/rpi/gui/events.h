#pragma once


#include "platform/platform.h"

/** Buttons available on the RCKid. 
 */
enum class Button {
    A, 
    B, 
    X, 
    Y, 
    L, 
    R, 
    Left, 
    Right,
    Up,
    Down, 
    Select, 
    Start, 
    Home, 
    VolumeUp, 
    VolumeDown, 
    Joy, 
}; // Button

struct ButtonEvent {
    Button btn;
    bool state;
}; // ButtonEvent

/** GUI event.

    The gui event is effectively a tagged union over the various event types supported by the gui.  
 */
struct Event {
public:
    enum class Kind {
        None, 
        Button, 
    }; // Event::Kind

    Kind kind;

    Event():kind{Kind::None} {}

    Event(Button button, bool state):kind{Kind::Button}, button_{button, state} {}

    ButtonEvent & button() {
        ASSERT(kind == Kind::Button);
        return button_;
    }

private:

    union {
        ButtonEvent button_;
    }; 

}; // Event