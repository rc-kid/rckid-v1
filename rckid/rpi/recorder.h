#pragma once

#include "widget.h"
#include "window.h"

class Recorder : public Widget {
public:
    Recorder(Window * window): Widget{window} {}


protected:

    void onBlur() {
        window()->rckid()->stopRecording();
    }

    void draw() {

    }

    void btnA(bool state) {
        if (state) {
            f_ = std::ofstream{"/rckid/recording.txt"};
            window()->rckid()->startRecording([this](RecordingEvent & e) {
                f_ << e.status.batchIndex() << ":";
                for (size_t i = 0; i < 32; ++i)
                    f_ <<  " " << e.data[i];
                f_ << std::endl;
            });
        } else {
            window()->rckid()->stopRecording();
            f_.close();
        }
    }

    std::ofstream f_;
}; // Recorder