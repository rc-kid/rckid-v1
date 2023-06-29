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
     
        The raw encoder is a specialzed direct opus codec encoder tuned for transmitting voice over the NRF packets. It wraps the opus packets of max 30 bytes with 2 bytes of extra information - the length of the opus packet and a packet index that can be used to detect multiple sends of the same packet as well as packet loss.
     */
    class RawEncoder {
    public:
        /** Creates new encoder. 
         
         */
        RawEncoder() {
            int err;
            encoder_ = opus_encoder_create(8000, 1, OPUS_APPLICATION_VOIP, &err);
            if (err != OPUS_OK)
                throw OpusError{STR("Unable to create opus encoder, code: " << err)};
            opus_encoder_ctl(encoder_, OPUS_SET_BITRATE(6000));                
        }

        ~RawEncoder() {
            opus_encoder_destroy(encoder_);
        }

        /** Encodes the provided buffer. 
         
            Returns true if there has been a new encoded frame created during the recording, in which case the call should be followed by sending the frame packet. 
         */
        bool encode(uint8_t const * data, size_t len) {
            frame_[0] = 0;
            while (len-- > 0) {
                buffer_.push_back(static_cast<opus_int16>(*(data++)) - 128 * 256);
                if (buffer_.size() == RAW_FRAME_LENGTH) {
                    ++frame_[1];
                    int result = opus_encode(encoder_, buffer_.data(), buffer_.size(), frame_ + 2, sizeof(frame_) - 2);
                    if (result > 0)
                        frame_[0] = result & 0xff;
                    else
                        throw OpusError{STR("Unable to encode frame, code:  " << result)};
                    buffer_.clear();
                }
            }
            return frame_[0] != 0;
        }

        /** Returns the encoded frame buffer.
         */
        unsigned char const * currentFrame() const {
            return frame_;
        }

        /** Returns the actual size of the opus frame encoded within the buffer, or 0 if the buffer is not valid at this time. 
        */
        size_t currentFrameSize() const {
            return frame_[0];
        }

        /** Returns the frame index. This is a monotonically increasing number wrapper around single byte so that we can send same packet multiple times for greater reach.
         */
        uint8_t currentFrameIndex() const {
            return frame_[1];
        }

    private:
        /** Number of samples in the unencoded frame. Corresponds to 8000Hz sample rate and 40ms window time, which at 6kbps gives us 30 bytes length encoded frame. 
         */
        static constexpr size_t RAW_FRAME_LENGTH = 320;
        OpusEncoder * encoder_;

        std::vector<opus_int16> buffer_;
        // 32 byte packets actually fit in single NRF message
        unsigned char frame_[32];
    }; // opus::RawEncoder

    /** Decoder ofthe raw packets encoded via the RawEncoder. 
     
        Together with the decoding also keeps track of packet loss and uses them to calculate approximate signal loss. 
     */
    class RawDecoder {
    public:
        RawDecoder(size_t expectedRetransmit = 1):
            expectedRetransmit_{expectedRetransmit} {
            int err;
            decoder_ = opus_decoder_create(8000, 1, &err);
            if (err != OPUS_OK)
                throw OpusError{STR("Unable to create opus decoder, code: " << err)};
        }

        ~RawDecoder() {
            opus_decoder_destroy(decoder_);
        }

        void reportPacketLoss(size_t numPackets = 1) {
            while (numPackets-- != 0) {
                int result = opus_decode(decoder_, nullptr, 0, buffer_, 320, false);
                if (result < 0)
                    throw OpusError{STR("Unable to decode missing packet, code: " << result)};
            }
        }

        size_t decodePacket(unsigned char const * rawPacket) {
            int result = opus_decode(decoder_, rawPacket + 2, rawPacket[0], buffer_, 320, false);
            if (result < 0)
                throw OpusError{STR("Unable to decode packet, code: " << result)};
            return result;
        }

        opus_int16 const * buffer() const { return buffer_; }

    private:
        OpusDecoder * decoder_;
        size_t expectedRetransmit_;

        opus_int16 buffer_[320];
    }; 

} // namespace 


// https://gitlab.xiph.org/xiph/opus-tools/-/blob/master/src/opusenc.c
// opusenc --raw --raw-bits 8 --raw-rate 8000 --raw-chan 1 --bitrate 6 --framesize 60 recording.dat 6kbps60.opus

    /*
    opus::RawEncoder enc = opus::RawEncoder{};
    opus::RawDecoder dec = opus::RawDecoder{};
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
            //opus_int16 * buffer = dec.decodePacket(enc.currentFrame());
            opus_int16 * buffer = dec.packetLost();
            for (int i = 0; i < 320; ++i)
                std::cout << buffer[i] << ",";
            std::cout << std::endl;
        }
    }
    std::cout << "Total encoded size: " << encodedSize << std::endl;
    return EXIT_SUCCESS;
    */