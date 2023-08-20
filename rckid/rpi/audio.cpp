#include "window.h"

#include "audio.h"

void AudioVisualizer::draw(Canvas & c, int left, int top, int width, int height) {
    size_t p = pos_;
    int barWidth = width / numBars_;
    left += (width - barWidth * numBars_) / 2; // center
    for (size_t i = 0; i < numBars_; ++i) {
        int t = (top + height) - (maxs_[p]) * height / 255;
        int d = (maxs_[p] - mins_[p]);
        int h = d * height / 255;
        DrawRectangle(left + barWidth * i, t, barWidth, h, ColorAlpha(c.accentColor(), 0.5 + d/512.0));
        p = (p + 1) % numBars_;             
    }
}
