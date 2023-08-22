#pragma once

#include <unordered_map>

#include "widget.h"
#include "window.h"
#include "audio.h"

/** Walkie Talkie
 
    A simple NRF24L01 based walkie talkie. The primary purpose of the walkie talkie is to send and receive real-time raw opus encoded audio (no headers) similar to a PTT walkie-talkie.  

    000 = PTT data
    ... = reserved
    111 = Control messages



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

    enum class Mode {
        Listening, 
        Recording, 
        Playing,
    }; 

    void setFooterHints() override {
        Widget::setFooterHints();
        window().addFooterItem(FooterItem::A("Talk"));
        window().addFooterItem(FooterItem::X("Beep"));
        window().addFooterItem(FooterItem::UpDown("󰕾"));
    }

    void tick() override {
        if (tHeartbeat_.update()) {
            tHeartbeat_.startRandom(WALKIE_TALKIE_HEARTBEAT_INTERVAL_MIN, WALKIE_TALKIE_HEARTBEAT_INTERVAL_MAX);
            if (mode_ == Mode::Listening) {
                uint8_t packet[32];
                new (packet) Heartbeat{heartbeatIndex_++, name_};
                rckid().nrfTransmitImmediate(packet);
            }
        }
    }

    void draw(Canvas & c) override {
        c.drawTexture(0, 20, icon_);
        c.drawText(70, 25, name_, WHITE, c.titleFont());
        switch (mode_) {
            case Mode::Listening: {
                c.drawTexture(25, 100, friends_);
                int y = 105;
                auto i = conns_.begin();
                while (i != conns_.end()) {
                    c.blendAdditive();
                    size_t q = i->second.quality();
                    if (q == 0) {
                        i = conns_.erase(i);
                    } else {
                        if (q > 80) {
                            c.drawText(70, y, "  ", c.accentColor());
                        } else if (q > 60) {
                            c.drawText(70, y, "  ", c.accentColor());
                        } else if (q > 40) {
                            c.drawText(70, y, "", c.accentColor());
                        } else {
                            c.drawText(70, y, "", c.accentColor());
                        }
                        c.blendAddColors();
                        c.drawText(130, y, i->first, LIGHTGRAY);
                        y += 20;
                        ++i;
                    }
                }
                break;
            }
            case Mode::Recording: {
                c.blendAlpha();
                c.drawFrame(5, 90, 310, 125, c.accentColor());
                c.blendAdditive();
                avis_.draw(c, 12, 150, 200, 70);
                c.drawTexture(225, 120, mic_);
                size_t sec = asMillis(now() - tStart_) / 1000;
                c.drawText(20, 105, STR("Recording... (" << sec << "s)"), WHITE, c.defaultFont());
                c.drawText(20, 125, STR("Up: " << packetsTx_ << ", Qs: " << rckid().nrfTxQueueSize()), LIGHTGRAY, c.helpFont());
                c.drawText(20, 140, STR("Sr: " << rawLength_ << ", Sc: " << compressedLength_), LIGHTGRAY, c.helpFont());
                break;
            }
            case Mode::Playing: {
#if (defined WALKIE_TALKIE_STORE_PTT)
                static int x = 0;
                if (pttIn_.eof()) {
                    if (!pttRxDone_) {
                        auto pttEnd = CmdWithName::PTTEnd("Foobar");
                        leavePlayingMode(pttEnd);
                        x = 0;
                        pttIn_.close();
                    }
                } else {
                    uint8_t buf[32];
                    for (int i = 0; i < 50 && !pttIn_.eof(); ++i) {
                        pttIn_.read(reinterpret_cast<char*>(buf), 32);
                        processPTTData(buf);
                        ++x;
                    }
                    std::cout << "We are at packet " << x << ", index " << (int)buf[1] << std::endl;
                }
#endif
                c.blendAlpha();
                c.drawFrame(5, 90, 310, 125, c.accentColor());
                c.blendAdditive();
                size_t sec = asMillis(now() - tStart_) / 1000;
                c.drawText(20, 105, STR("Playing... (" << sec << "s)"), WHITE, c.defaultFont());
                c.drawText(20, 125, STR("Down: " << packetsRx_ << ", Qs: " << rxAudioBuffers_.size()), LIGHTGRAY, c.helpFont());
                c.drawText(20, 140, STR("Pc: " << dec_.packets() <<  ", Pe: " << dec_.missingPackets()), LIGHTGRAY, c.helpFont());
                if (IsAudioStreamProcessed(pttRx_) && ! rxAudioBuffers_.empty()) {
                    UpdateAudioStream(pttRx_, rxAudioBuffers_.front(), 960);
                    delete [] rxAudioBuffers_.front();
                    rxAudioBuffers_.pop_front();
                }
                if (pttRxDone_) {
                    if (IsAudioStreamProcessed(pttRx_) && rxAudioBuffers_.empty()) {
                        mode_ = Mode::Listening;
                        StopAudioStream(pttRx_);
                        std::cout << " Stream stopped" << std::endl;
                        UnloadAudioStream(pttRx_);
                        std::cout << " Stream unloaded" << std::endl;
                    }
                } 
               break;
            }
        }
    }


    void onFocus() override {
        rckid().nrfInitialize("RCKid", "RCKid", channel_);
        conns_.clear();
        rckid().nrfEnableReceiver();
        mode_ = Mode::Listening;
        tHeartbeat_.startRandom(WALKIE_TALKIE_HEARTBEAT_INTERVAL_MIN, WALKIE_TALKIE_HEARTBEAT_INTERVAL_MAX);
        /*
        Heartbeat h{0, "Franta"};
        for (int i = 0; i < 16; ++i) {
            h.index = i * 2;
            processIncomingHeartbeat(h);
        }
        */
    }

    void onBlur() override {
        tHeartbeat_.stop();
        rckid().stopAudioRecording();
        mode_ = Mode::Listening;
        rckid().nrfPowerDown();
    }

    /** Starts / stops the PTT Transmission. 
     */
    void btnA(bool state) override {
        if (mode_ == Mode::Listening && state) {
            mode_ = Mode::Recording;
            enc_.reset();
            rawLength_ = 0;
            compressedLength_ = 0;
            packetsTx_ = 0;
            {
                rckid().nrfEnableTransmitter();
                auto msg = CmdWithName::PTTStart(name_);
                rckid().nrfTransmit( & msg, 32);
            }
            avis_.reset();
            // tell everyone we will begin PTT
            rckid().startAudioRecording();
            tStart_ = now();
#if (defined WALKIE_TALKIE_STORE_PTT)
            pttOut_ = std::ofstream{"/rckid/ptt.dat", std::ios::binary};
            if (!pttOut_.good())
                std::cout << "Error creating oustream" << std::endl;
#endif
        } else if (mode_ == Mode::Recording && !state) {
            rckid().stopAudioRecording();
            rckid().nrfReset();
            mode_ = Mode::Listening;
            rckid().nrfEnableReceiver();
            {
                auto msg = CmdWithName::PTTEnd(name_);
                rckid().nrfTransmitImmediate( & msg, 32);
            }
#if (defined WALKIE_TALKIE_STORE_PTT)
            pttOut_.close();
#endif
        }
    }

    /** Sends the BEEP msg. 
     */
    void btnX(bool state) override {
        if (state) {
            //rckid().nrfTransmit(msg_, true);
        }
    }

    void dpadUp(bool state) override {
        if (state)
            rckid().setVolume(rckid().volume() + 10);
    }

    void dpadDown(bool state) override {
        if (state)
            rckid().setVolume(rckid().volume() - 10);
    }

#if (defined WALKIE_TALKIE_STORE_PTT)
    /** Debug only, starts fake playback */
    void btnStart(bool state) override {
        if (state && mode_ == Mode::Listening) {
            auto pttStart = CmdWithName::PTTStart("Foobar");
            enterPlayingMode(pttStart);
            pttIn_ = std::ifstream{"/rckid/ptt.dat", std::ios::binary};
        }
    }
#endif

    void audioRecorded(RecordingEvent & e) override {
        avis_.addData(e.data, 32);
        rawLength_ += 32;
        if (enc_.encode(e.data, 32))
            compressedLength_ += enc_.currentFrameSize();
        if (enc_.currentFrameValid()) {
            rckid().nrfTransmit(enc_.currentFrame(), 32);
#if (defined WALKIE_TALKIE_STORE_PTT)
            pttOut_.write(reinterpret_cast<char const *>(enc_.currentFrame()), 32);
#endif
        }
    }

    void nrfPacketReceived(NRFPacketEvent & e) override {
        switch (e.packet[0]) {
            case MSG_HEARTBEAT:
                processIncomingHeartbeat(*reinterpret_cast<Heartbeat*>(e.packet));
                break;
            case MSG_PTT_START:
                enterPlayingMode(*reinterpret_cast<CmdWithName*>(e.packet));
                break;
            case MSG_PTT_END:
                leavePlayingMode(*reinterpret_cast<CmdWithName*>(e.packet));
                break;
            case MSG_BEEP:
                // TODO
                break; 
            default:
                // TODO if not playing, enter playing mode
                processPTTData(e.packet);
                break;
        }
    }


    void nrfTxDone() override {
        if (mode_ == Mode::Recording)
            ++packetsTx_;
    }

private:

    static constexpr uint8_t MSG_VOICE_MASK = 0b00011111;
    static constexpr uint8_t MSG_HEARTBEAT  = 0b11100000;
    static constexpr uint8_t MSG_PTT_START  = 0b11100001;
    static constexpr uint8_t MSG_PTT_END    = 0b11100010;
    static constexpr uint8_t MSG_BEEP       = 0b11100011; 

    struct CmdWithName {
        uint8_t const id;
        char name[31];

        static CmdWithName PTTStart(std::string const & name) { return CmdWithName{MSG_PTT_START, name}; }

        static CmdWithName PTTEnd(std::string const & name) { return CmdWithName{MSG_PTT_END, name}; }

    private:
        CmdWithName(uint8_t id, std::string const & name):
            id{id} {
            memcpy(this->name, name.c_str(), std::min(name.size() + 1, (size_t)31));
            this->name[30] = 0; // ensure null termination of the name string
        }
    } __attribute__((packed));

    static_assert(sizeof(CmdWithName) == 32);

    struct Heartbeat {
        uint8_t const id = MSG_HEARTBEAT;
        uint8_t index;
        char name[30];

        Heartbeat(uint8_t index, std::string const & name):
            index{index} {
            memcpy(this->name, name.c_str(), std::min(name.size() + 1, (size_t)30));
            this->name[29] = 0; // ensure null termination of the heartbeat string
        }

    } __attribute__((packed)); 

    static_assert(sizeof(Heartbeat) == 32);

    /** For each device in range we keep the number of heartbeats we have received */
    struct ConnectionInfo {
        static constexpr size_t DEPTH = 16;
        std::deque<bool> seen;
        size_t valid = 0;
        uint8_t lastIndex;
        Timepoint lastUpdate;
        
        void addIndex(uint8_t index) {
            lastUpdate = now();
            ++lastIndex;
            while (lastIndex != index) {
                seen.push_back(false);
                ++lastIndex;
            }
            seen.push_back(true);
            ++valid;
            while (seen.size() > DEPTH) {
                if (seen.front())
                    --valid;
                seen.pop_front();
            }
        }

        size_t quality() {
            Timepoint t = now();
            size_t adj = asMillis(t - lastUpdate) / WALKIE_TALKIE_HEARTBEAT_INTERVAL_MAX;
            if (adj > valid)
                return 0;
            return (valid - adj) * 100 / DEPTH;
        }
    }; 

    void processIncomingHeartbeat(Heartbeat const & msg) {
        std::string name{msg.name};
        auto i = conns_.find(name);
        if (i == conns_.end())
            i = conns_.insert(std::make_pair(name, ConnectionInfo{})).first;
        i->second.addIndex(msg.index);
    }

    void processPTTData(uint8_t * data) {
        ++packetsRx_;
        size_t n = dec_.decodePacket(data);
        if (n != 0) {
            if (rxAudioBufferSize_ == 0)
                rxAudioBuffer_ = new int16_t[960];
            memcpy(rxAudioBuffer_ + rxAudioBufferSize_, dec_.buffer(), 320 * 2);
            rxAudioBufferSize_ += 320;
            if (rxAudioBufferSize_ == 960) {
                rxAudioBufferSize_ = 0;
                rxAudioBuffers_.push_back(rxAudioBuffer_);
                rxAudioBuffer_ = nullptr;
            }
        }
    }

    void enterPlayingMode(CmdWithName const & cmd) {
        // ignore if we are already in playing mode
        if (mode_ == Mode::Playing)
            return;
        mode_ = Mode::Playing;
        senderName_ = cmd.name;
        pttRxDone_ = false;
        // start audio playback
        SetAudioStreamBufferSizeDefault(960); // 3 frames for 12.5 fps minimum 
        pttRx_ = LoadAudioStream(8000, 16, 1);
        SetAudioStreamBufferSizeDefault(0); // reset
        PlayAudioStream(pttRx_);
        dec_.reset();
        rxAudioBuffers_.clear();
        tStart_ = now();
        packetsRx_ = 0;
    }

    void leavePlayingMode(CmdWithName const & cmd) {
        // don't do anything if we are not playing in the first place
        if (mode_ != Mode::Playing)
            return;
        // if the message is from someone else, ignore it too
        std::string sender{cmd.name};
        if (sender != senderName_)
            return;
        // otherwise we are done receiving, wait for the playback to be finished and then stop
        pttRxDone_ = true;
        if (rxAudioBuffer_ != nullptr) {
            delete [] rxAudioBuffer_;
            rxAudioBuffer_ = nullptr;
            rxAudioBufferSize_ = 0;
        }
    }


    uint8_t channel_{86};
    std::string name_{"Ada"};

    // Name from which we are currently receiving. Only useful in playing mode 
    std::string senderName_{};
    bool pttRxDone_;
    AudioStream pttRx_;
    int16_t * rxAudioBuffer_{nullptr};
    size_t rxAudioBufferSize_{0};
    std::deque<int16_t*> rxAudioBuffers_;

    // heartbeat timer so that we send our heartbeats in semi-regular intervals
    Timer tHeartbeat_;
    // heartbeat index so that we know how many we have out of how much
    uint8_t heartbeatIndex_ = 0;
    // list of devices we have received heartbeat from
    std::unordered_map<std::string, ConnectionInfo> conns_;

    Mode mode_{Mode::Listening};
    Timepoint tStart_;


    size_t rawLength_;
    size_t compressedLength_;
    size_t packetsTx_; 
    size_t packetsRx_ = 0; 
    AudioVisualizer avis_{8000, 30, 0.5};
    opus::RawEncoder enc_;
    opus::RawDecoder dec_;

    Canvas::Texture icon_{"assets/icons/baby-monitor-64.png"};
    Canvas::Texture friends_{"assets/icons/people-32.png"};
    Canvas::Texture mic_{"assets/icons/microphone-64.png"};



#if (defined WALKIE_TALKIE_STORE_PTT)
    std::ofstream pttOut_;
    std::ifstream pttIn_;
#endif

}; // WalkieTalkie