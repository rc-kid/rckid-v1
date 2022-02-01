#pragma once

#include "platform/gpio.h"

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
        outOffset_ = pio_add_program(pio_, &i2s_out_program);
        outSm_ = pio_claim_unused_sm(pio_, true);
        setOutSampleRate(SampleRate::khz8);
        i2s_out_program_init(pio_, outSm_, outOffset_, i2sData, i2sClock);
        /*
        uint16_t i = 0;
        while (true) {
            outMono(i);
            i = i + 1;
        }
        */
    }

    void setOutSampleRate(SampleRate sr) {
        switch (sr) {
            case SampleRate::khz8:
                pio_sm_set_clkdiv_int_frac(pio_, outSm_, 259, 136);
                break;
            case SampleRate::khz44_1:
                pio_sm_set_clkdiv_int_frac(pio_, outSm_, 519, 136);
                break;
            case SampleRate::khz48:
                pio_sm_set_clkdiv_int_frac(pio_, outSm_, 519, 136);
                break;
        }
    }

    void outMono(uint16_t value) {
        while (pio_sm_is_tx_fifo_full(pio_, outSm_));
        pio_->txf[outSm_] = (value << 16) | value;
    }   

    void outStereo(uint16_t l, uint16_t r) {
        while (pio_sm_is_tx_fifo_full(pio_, outSm_));
        pio_->txf[outSm_] = (r << 16) | l;
    } 

private:

    PIO pio_;
    uint outSm_;
    uint outOffset_;

};