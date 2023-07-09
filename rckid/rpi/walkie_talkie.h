#pragma once


#include "widget.h"
#include "window.h"
#include "opus.h"




/** Walkie Talkie
 
    A simple NRF24L01 based walkie talkie. The primary purpose of the walkie talkie is to send and receive real-time raw opus encoded audio (no headers) similar to a PTT walkie-talkie.  

    000xxxxx yyyyyyyy = opus data packet, x = packet size, y = packet index
    
    1xxxxxxx = special command. Can be one of:

    beep =
    voice start (who)
    voice end (who)
    data start (what img)
    data packet
    data end
    data failure 
    heartbeat (?)

    # Heartbeats

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
        window()->rckid()->nrfInitialize("AAAAA", "AAAAA", 86);
        window()->rckid()->nrfEnableReceiver();
    }

    void onBlur() override {
        window()->rckid()->nrfStandby();
    }

    void btnA(bool state) override {

    }

    /** Sends the BEEP msg. 
     */
    void btnX(bool state) override {
        if (state) {
            window()->rckid()->nrfTransmit(msgBeep_, true);
        }
    }

private:

    opus::RawEncoder enc_;
    opus::RawDecoder dec_;

    uint8_t msgBeep_[32] = { 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

}; // WalkieTalkie