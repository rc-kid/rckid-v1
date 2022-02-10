#pragma once

#include "platform/gpio.h"

#include <hardware/clocks.h>


#include "i2s.pio.h"

/* Using Codec2/OPUS for the walkie talkie, Opus can also be used for playback easily, the quality is there. 
 */
class Audio {
public:

    enum class SampleRate {
        khz8,
        khz44_1,
        khz48
    }; 

    /** Initializes the audio system.
     */
    void initialize(PIO pio, gpio::Pin i2sData, gpio::Pin i2sClock) {
        pio_ = pio;
        outOffset_ = pio_add_program(pio_, &i2s_out_lsbj_program);
        outSm_ = pio_claim_unused_sm(pio_, true);
        i2s_out_program_init(pio_, outSm_, outOffset_, i2sData, i2sClock);
        setOutSampleRate(SampleRate::khz8);
        int16_t i = 0;
        while (true) {
            outStereo(i > 0 ? 32767 : -32768, i);
            //outStereo(i, 65535 - i);
            //outStereo((i > 32768) ? 32768 : 1024, i);
            //outStereo(0x0000,0xffff);
            i = i + 8;
        }
    }

    void setOutSampleRate(uint hz) {
        uint clk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS) * 1000; // [Hz]
        hz = hz * 16 * 2 *2; // 16 bits per channel, 2 channels, 2 pio instructions per bit
        uint clkdiv = clk / hz;
        uint clkfrac = (clk - (clkdiv * hz)) * 256 / hz;
        pio_sm_set_clkdiv_int_frac(pio_, outSm_, clkdiv & 0xffff, clkfrac & 0xff);
    }

    void setOutSampleRate(SampleRate sr) {
        switch (sr) {
            case SampleRate::khz8:
                setOutSampleRate(8000);
                break;
            case SampleRate::khz44_1:
                setOutSampleRate(44100);
                break;
            case SampleRate::khz48:
                setOutSampleRate(48000);
                break;
        }
    }

    void outMono(int16_t value) {
        while (pio_sm_is_tx_fifo_full(pio_, outSm_));
        pio_->txf[outSm_] = (static_cast<uint16_t>(value) << 16) | static_cast<uint16_t>(value);
    }   

    void outStereo(int16_t l, int16_t r) {
        while (pio_sm_is_tx_fifo_full(pio_, outSm_));
        pio_->txf[outSm_] = (static_cast<uint16_t>(r) << 16) | static_cast<uint16_t>(l);
    } 

private:

    PIO pio_;
    uint outSm_;
    uint outOffset_;

};