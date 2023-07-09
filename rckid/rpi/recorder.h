#pragma once

#include "widget.h"
#include "window.h"

class Recorder : public Widget {
public:
    Recorder(Window * window): Widget{window} {}

protected:

    void onBlur() override {
        window()->rckid()->stopRecording();
        recording_ = false;
    }

    void draw() override {
        int i = 0;
        int x = maxIndex_;
        while (i < 320) {
            int max = max_[x] / 2;
            int min = min_[x] / 2;
            DrawLine(i, 184 - max, i, 184 - min, GREEN);
            x = (x + 1) % 320;
            ++i;
        }

        if (recording_)
            DrawTextEx(window()->helpFont(), "Recording...", 0, 25, 16, 1.0, WHITE);
    }

    void btnA(bool state) override {
        if (state) {
            if (! recording_) {
                recording_ = true;
                f_ = std::ofstream{"/rckid/recording.dat", std::ios::binary};
                maxIndex_ = 0;
                nextIndex_ = 0;
                memset(max_, 128, 320);
                memset(min_, 128, 320);
                window()->rckid()->startRecording();
            }
        } else {
            recording_ = false;
            window()->rckid()->stopRecording();
            f_.close();
        }
    }

    void audioRecorded(RecordingEvent & e) {
        while (e.status.batchIndex() != nextIndex_) {
            nextIndex_ = (nextIndex_ + 1) % 8;
            f_.write(reinterpret_cast<char const *>(empty_), 32);
        }
        nextIndex_ = (nextIndex_ + 1) % 8;
        //f_ << (int)e.status.batchIndex() << ":";
        f_.write(reinterpret_cast<char const *>(e.data), 32);
        uint8_t max = 0;
        uint8_t min = 255;
        for (size_t i = 0; i < 32; ++i) {
            //f_ <<  " " << (int)e.data[i];
            if (e.data[i] > max)
                max = e.data[i];
            if (e.data[i] < min)
                min = e.data[i];
        }
        min_[maxIndex_] = min;
        max_[maxIndex_] = max;
        maxIndex_ = (maxIndex_ + 1) % 320;
        //f_ << std::endl;
    }

    std::ofstream f_;
    uint8_t min_[320];
    uint8_t max_[320];
    size_t maxIndex_ = 0;
    uint8_t nextIndex_ = 0;
    bool recording_ = false;
    uint8_t empty_[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
}; // Recorder