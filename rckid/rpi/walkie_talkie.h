#pragma once

#include <unordered_map>

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

protected:

    void tick() override {
        if (tHeartbeat_.update()) {
            tHeartbeat_.startRandom(WALKIE_TALKIE_HEARTBEAT_INTERVAL_MIN, WALKIE_TALKIE_HEARTBEAT_INTERVAL_MAX);
            if (!recording_ && !receiving_) {
                uint8_t packet[32];
                new (packet) Heartbeat{heartbeatIndex_++, name_};
                window().rckid()->nrfTransmit(packet);
            }
        }
    }

    void draw() override {
        if (recording_) {
            avis_.draw(10, 150, 300, 70);
            DrawRectangleLines(10, 150, 300, 70, WHITE);
        }
        DrawText(STR(rawLength_).c_str(), 0, 30, 20, WHITE);
        DrawText(STR(compressedLength_).c_str(), 0, 50, 20, WHITE);
        DrawText(STR(packetsSent_).c_str(), 0, 70, 20, WHITE);
        DrawText(STR(packetErrors_).c_str(), 0, 90, 20, RED);
        DrawText(STR(window().rckid()->nrfTxQueueSize()).c_str(), 0, 110, 20, BLUE);
        DrawText(STR(packetsReceived_).c_str(), 0, 130, 20, GREEN);
    }


    void onFocus() override {
        window().rckid()->nrfInitialize("AAAAA", "AAAAA", 86);
        window().rckid()->nrfEnableReceiver();
        recording_ = false;
        tHeartbeat_.startRandom(WALKIE_TALKIE_HEARTBEAT_INTERVAL_MIN, WALKIE_TALKIE_HEARTBEAT_INTERVAL_MAX);
    }

    void onBlur() override {
        tHeartbeat_.stop();
        window().rckid()->stopRecording();
        recording_ = false;
        window().rckid()->nrfStandby();
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
                window().rckid()->startRecording();
            }
        } else {
            if (recording_) {
                recording_ = false;
                window().rckid()->stopRecording();
            }
        }

    }

    /** Sends the BEEP msg. 
     */
    void btnX(bool state) override {
        if (state) {
            //window().rckid()->nrfTransmit(msg_, true);
        }
    }

    void audioRecorded(RecordingEvent & e) override {
        avis_.addData(e.data, 32);
        rawLength_ += 32;
        if (enc_.encode(e.data, 32))
            compressedLength_ += enc_.currentFrameSize();
        if (enc_.currentFrameValid())
            window().rckid()->nrfTransmit(enc_.currentFrame(), 32);
    }

    void nrfPacketReceived(NRFPacketEvent & e) override {
        ++packetsReceived_; 
        switch (e.packet[0]) {
            case MSG_HEARTBEAT:
                processIncomingHeartbeat(*reinterpret_cast<Heartbeat*>(e.packet));
                break;
            case MSG_BEEP:
                // TODO
                break; 
            default:
                // TODO voice PTT
                break;
        }
    }


    void nrfTxCallback(bool ok) override {
        // should be always ok
        if (ok) 
            ++packetsSent_;
        else 
            ++packetErrors_;
    }

private:

    static constexpr uint8_t MSG_VOICE_MASK = 0x1f;
    static constexpr uint8_t MSG_HEARTBEAT = 0x80;
    static constexpr uint8_t MSG_BEEP = 0xff; 

    struct Heartbeat {
        uint8_t const id = MSG_HEARTBEAT;
        uint8_t index;
        char name[];

        Heartbeat(uint8_t index, std::string const & name):
            index{index} {
            memcpy(this->name, name.c_str(), std::min(name.size(), (size_t)30));
            this->name[30] = 0; // ensure null termination of the heartbeat string
        }

    } __attribute__((packed)); 

    /** For each device in range we keep the number of heartbeats we have received */
    struct ConnectionInfo {
        std::vector<bool> seen_;

    }; 

    void processIncomingHeartbeat(Heartbeat const & msg) {

    }


    std::string name_{"RCKid"};

    // heartbeat timer so that we send our heartbeats in semi-regular intervals
    Timer tHeartbeat_;
    // heartbeat index so that we know how many we have out of how much
    uint8_t heartbeatIndex_ = 0;
    // list of devices we have received heartbeat from
    std::unordered_map<std::string, ConnectionInfo> conns_;



    bool recording_ = false;
    bool receiving_ = false;
    size_t rawLength_;
    size_t compressedLength_;
    size_t packetsSent_; 
    size_t packetErrors_;
    size_t packetsReceived_ = 0; 
    AudioVisualizer avis_{8000, 60, 1};
    opus::RawEncoder enc_;
    opus::RawDecoder dec_;

}; // WalkieTalkie