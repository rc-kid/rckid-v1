#pragma once

#include <deque>

#include "widget.h"
#include "window.h"

/** A simple NRF sniffer class that displays received messages, which is useful for testing
    
 */
class NRFSniffer : public Widget {
public:

protected:

    void draw() override {
        DrawTextEx(window().helpFont(), STR("TX: " << tx_ << " RX: " << rx_).c_str(), 0, 200, 16, 1.0, WHITE);
        BeginBlendMode(BLEND_ADD_COLORS);
        for (size_t i = 0; i < msgs_.size(); ++i) {
            size_t id = (rx_ - msgs_.size() + i) % 1000;
            DrawTextEx(window().helpFont(), STR(id << ":").c_str(), 0, 20 + 18 * i, 16, 1.0, DARKGRAY);
            DrawTextEx(window().helpFont(), msgs_[i].bytesToStr(0, 8).c_str(), 32, 20 + 18 * i, 16, 1.0, WHITE);
            DrawTextEx(window().helpFont(), ":", 145, 20 + 18 * i, 16, 1.0, DARKGRAY);
            DrawTextEx(window().helpFont(), msgs_[i].bytesToStr(8, 8).c_str(), 153, 20 + 18 * i, 16, 1.0, WHITE);
        }
    }

    void onFocus() override {
        window().addFooterItem(FooterItem::A("Start/Stop"));
        window().addFooterItem(FooterItem::X("Reset"));
    }

    void onNavigationPush() override {

        rckid().nrfInitialize(rxAddr_.c_str(), txAddr_.c_str(), channel_);
        rckid().nrfEnableReceiver();
        /*
        if (msgs_.empty()) {
            msgs_.push_back(Message{"0123456789", 10});
            ++tx_;
        } */
    }

    void onNavigationPop() override {
        rckid().nrfStandby();
    }

    void btnA(bool state) override {
        if (state) {
            if (running_) {
                running_ = false;
                rckid().nrfStandby();
            } else {
                running_ = true;
                rckid().nrfInitialize(rxAddr_.c_str(), txAddr_.c_str(), channel_);
                rckid().nrfEnableReceiver();
            }
        }
    }

    void btnX(bool state) override {
        if (state) {
            msgs_.clear();
            rx_ = 0;
            tx_ = 0;
        }
    }

    void nrfPacketReceived(NRFPacketEvent & e) override {
        ++rx_;
        msgs_.push_back(Message{e.packet});
        while (msgs_.size() > 8)
            msgs_.pop_front();
    }

private:

    struct Message {
        uint8_t packet[32];

        Message(uint8_t const * packet, size_t length = 32) {
            memcpy(this->packet, packet, length);
        }

        Message(char const * packet, size_t length = 32) {
            memcpy(this->packet, packet, length);
        }

        std::string bytesToStr(size_t start, size_t num) {
            std::stringstream ss;
            while (num-- > 0) {
                ss << "0123456789abcdef"[packet[start] / 16];
                ss << "0123456789abcdef"[packet[start++] % 16];
            }
            return ss.str();
        }

    };

    bool running_ = true;
    std::string rxAddr_ = "AAAAA";
    std::string txAddr_ = "AAAAA";
    uint8_t channel_ = 86;
    size_t rx_ = 0;
    size_t tx_ = 0;

    std::deque<Message> msgs_;


}; 