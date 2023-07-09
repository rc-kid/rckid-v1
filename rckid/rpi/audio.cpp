#include "window.h"

#include "audio.h"

void AudioVisualizer::draw(Window * window, int top, int left, int width, int height) {
    size_t p = pos_;
    int barWidth = width / numBars_;
    for (size_t i = 0; i < numBars_; ++i) {
        int t = (255 - maxs_[p]) * height / 255 + top;
        int h = (maxs_[p] - mins_[p]) * height / 255;
        DrawRectangle(left + barWidth * i, t, barWidth, h, GREEN);
        p = (p + 1) % numBars_;             
    }
}
