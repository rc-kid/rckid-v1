#pragma once

#include "widget.h"
#include "window.h"

class Recorder : public Widget {
public:
    Recorder(Window * window): Widget{window} {}


protected:

    void onBlur() {
        window()->rckid()->stopRecording();
        recording_ = false;
    }

    void draw() {
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

    void btnA(bool state) {
        if (state) {
            if (! recording_) {
                recording_ = true;
                f_ = std::ofstream{"/rckid/recording.dat", std::ios::binary};
                maxIndex_ = 0;
                memset(& max_, 0, 320);
                window()->rckid()->startRecording([this](RecordingEvent & e) {
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
                });
            }
        } else {
            recording_ = false;
            window()->rckid()->stopRecording();
            f_.close();
        }
    }

    std::ofstream f_;
    uint8_t min_[320];
    uint8_t max_[320];
    size_t maxIndex_ = 0;
    bool recording_ = false;
}; // Recorder