#pragma once


#include "widget.h"
#include "window.h"
#include "opus.h"




/** Walkie Talkie
 
    A simple NRF24L01 based walkie talkie. It supports sending opus encoded voice in real-time as well as sending files (images). 

    000xxxxx yyyyyyyy = opus data packet, x = packet size, y = packet index
    
    1xxxxxxx = special command. Can be one of:

    voice start (who)
    voice end (who)
    data start (what img)
    data packet
    data end
    data failure 
    heartbeat (?)

 */
class WalkieTalkie : public Widget {
public:
    WalkieTalkie(Window * window): Widget{window} {}

protected:

    void draw() override {

    }

    void idle() override {

    }

    void onFocus() override {

    }

    void onBlur() override {
    }

    void btnA(bool state) override {

    }

private:

    opus::RawEncoder enc_;
    opus::RawDecoder dec_;

}; // WalkieTalkie