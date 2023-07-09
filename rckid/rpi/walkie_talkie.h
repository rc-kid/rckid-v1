#pragma once


#include "widget.h"
#include "window.h"
#include "audio.h"

/** Walkie Talkie
 
    A simple NRF24L01 based walkie talkie. The primary purpose of the walkie talkie is to send and receive real-time raw opus encoded audio (no headers) similar to a PTT walkie-talkie.  

    ! Have images as well

    000xxxxx yyyyyyyy = opus data packet, x = packet size, y = packet index
    
    1xxxxxxx = special command. Can be one of:


    PTT START | name 
    PTT END | name
    PTT 
    BEEP
    HEARTBEAT | name 
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
        if (recording_) {
            avis_.draw(window(), 10, 150, 300, 70);
            DrawRectangleLines(10, 150, 300, 70, WHITE);
        }
        DrawText(STR(rawLength_).c_str(), 0, 30, 20, WHITE);
        DrawText(STR(compressedLength_).c_str(), 0, 50, 20, WHITE);
        DrawText(STR(packetsSent_).c_str(), 0, 70, 20, WHITE);
        DrawText(STR(packetErrors_).c_str(), 0, 90, 20, RED);
        DrawText(STR(window()->rckid()->nrfTxQueueSize()).c_str(), 0, 110, 20, BLUE);
    }

    void idle() override {

    }

    void onFocus() override {
        window()->rckid()->nrfInitialize("AAAAA", "AAAAA", 86);
        window()->rckid()->nrfEnableReceiver();
        recording_ = false;
    }

    void onBlur() override {
        window()->rckid()->stopRecording();
        recording_ = false;
        window()->rckid()->nrfStandby();
    }

    /** Starts / stops the PTT Transmission. 
     */
    void btnA(bool state) override {
        if (state) {
            if (! recording_) {
                avis_.reset();
                recording_ = true;
                rawLength_ = 0;
                compressedLength_ = 0;
                packetsSent_ = 0;
                packetErrors_ = 0;
                // tell everyone we will begin PTT
                window()->rckid()->startRecording();
            }
        } else {
            if (recording_) {
                recording_ = false;
                window()->rckid()->stopRecording();
            }
        }

    }

    /** Sends the BEEP msg. 
     */
    void btnX(bool state) override {
        if (state) {
            //window()->rckid()->nrfTransmit(msg_, true);
        }
    }


    void audioRecorded(RecordingEvent & e) override {
        avis_.addData(e.data, 32);
        rawLength_ += 32;
        if (enc_.encode(e.data, 32))
            compressedLength_ += enc_.currentFrameSize();
        if (enc_.currentFrameValid())
            window()->rckid()->nrfTransmit(enc_.currentFrame(), 32);
    }

    void nrfTxCallback(bool ok) override {
        if (ok) 
            ++packetsSent_;
        else 
            ++packetErrors_;
    }


private:

    bool recording_ = false;
    size_t rawLength_;
    size_t compressedLength_;
    size_t packetsSent_; 
    size_t packetErrors_;
    AudioVisualizer avis_{8000, 60, 1};
    opus::RawEncoder enc_;
    opus::RawDecoder dec_;

}; // WalkieTalkie