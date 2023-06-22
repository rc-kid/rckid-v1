#pragma once
#include <vector>


#include <opus/opus.h>

#include "platform/platform.h"
#include "utils/utils.h"

namespace opus {

    class OpusError : public std::runtime_error {
    public:
        OpusError(std::string const & what): std::runtime_error{what} {}
    };

    enum class SampleRate : opus_int32 {
        khz8000 = 8000, 
        khz12000 = 12000, 
        khz24000 = 24000, 
        khz48000 = 48000,
    }; // opus::SampleRate

    /** Frame sizes supported by the encoder. 

        Encoded as the number of frames per second.  
     */
    enum class FrameSize : size_t {
        ms2_5 = 400, 
        ms5 = 200, 
        ms10 = 100, 
        ms20 = 50, 
        ms40 = 25, 
        ms16 = 17,
    }; // opus::FrameSize

    /** Raw Opus Encoder
     
        The raw encoder is a specialzed direct opus codec encoder tuned for transmitting voice over the NRF packets. It wraps the codec packets into 
     */
    class RawEncoder {
    public:
        /** Creates new encoder. 
         
            Default attributes are 8000kHz sample rate at which RCKid works, the lowest bitrate (6000) and frame size of 40ms. This gives us an encoded frame size of 30 bytes, which fits perfectly into a single NRF packet. 
         */
        RawEncoder(SampleRate sr = SampleRate::khz8000, size_t bitrate = 6000, FrameSize frameSize = FrameSize::ms40):
        sampleRate_{static_cast<opus_int32>(sr)},
        bitrate_{bitrate},
        framesPerSecond_{static_cast<size_t>(frameSize)} {
            int err;
            encoder_ = opus_encoder_create(sampleRate_, 1, OPUS_APPLICATION_VOIP, &err);
            if (err != OPUS_OK)
                throw OpusError{STR("Unable to create opus encoder, code: " << err)};
            opus_encoder_ctl(encoder_, OPUS_SET_BITRATE(bitrate_));                
        }

        ~RawEncoder() {
            opus_encoder_destroy(encoder_);
        }

        /** Encodes the provided buffer. 
         */
        size_t encode(uint8_t const * data, size_t len) {
            size_t rawFrameLength = sampleRate_ / framesPerSecond_;
            frameSize_ = 0;
            while (len-- > 0) {
                buffer_.push_back(static_cast<opus_int16>(*(data++)) - 128 * 256);
                if (buffer_.size() == rawFrameLength) {
                    int result = opus_encode(encoder_, buffer_.data(), buffer_.size(), frame_, sizeof(frame_));
                    if (result > 0)
                        frameSize_ = result;
                    else
                        throw OpusError{STR("Unable to encode frame, code:  " << result)};
                    buffer_.clear();
                }
            }
            return frameSize_;
        }

        unsigned char * currentFrame() {
            return frame_;
        }
    private:
        OpusEncoder * encoder_;
        opus_int32 sampleRate_;
        size_t bitrate_;
        size_t framesPerSecond_;

        std::vector<opus_int16> buffer_;
        // 32 byte packets actually fit in single NRF message
        unsigned char frame_[32];
        size_t frameSize_ = 0;
    }; // opus::Encoder

} // namespace 


// https://gitlab.xiph.org/xiph/opus-tools/-/blob/master/src/opusenc.c
// opusenc --raw --raw-bits 8 --raw-rate 8000 --raw-chan 1 --bitrate 6 --framesize 60 recording.dat 6kbps60.opus

    /*
    opus::RawEncoder enc = opus::RawEncoder{};
    uint8_t data[10];
    uint8_t x = 0;
    size_t encodedSize = 0;
    for (size_t i = 0; i < 800; ++i) {
        for (size_t j = 0; j < 10; ++j)
            data[j] = x++;
        size_t frameSize = enc.encode(data, 10);
        if (frameSize != 0) {
            std::cout << "After " << ((i + 1) * 10) << " samples, frame size: " << frameSize << std::endl;
            encodedSize += frameSize;
        }
    }
    std::cout << "Total encoded size: " << encodedSize << std::endl;
    return EXIT_SUCCESS;
    */
