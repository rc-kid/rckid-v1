#include "remote.h"

namespace remote {

    class LegoRemote {
    public:

        static constexpr uint8_t CHANNEL_ML = 1;
        static constexpr uint8_t CHANNEL_MR = 2;
        static constexpr uint8_t CHANNEL_L1 = 3;
        static constexpr uint8_t CHANNEL_R1 = 4;
        static constexpr uint8_t CHANNEL_L2 = 5;
        static constexpr uint8_t CHANNEL_R2 = 6;
        static constexpr uint8_t CHANNEL_TONE_EFFECT = 7;
        static constexpr uint8_t CHANNEL_RGB_STRIP = 8;

        /** Since the entire feedback of the Lego remote fits in 32 bytes, we can simply send all */
        class Feedback {
        private:
            msg::Kind const msg = msg::Kind::FeedbackConsecutive;
            uint8_t const startChannel = 1;
        public:
            channel::Motor::Feedback ml;
            channel::Motor::Feedback mr;
            channel::CustomIO::Feedback l1;
            channel::CustomIO::Feedback r1; 
            channel::CustomIO::Feedback l2;
            channel::CustomIO::Feedback r2;
            channel::ToneEffect::Feedback tone;
            channel::RGBStrip::Feedback rgb;
        } __attribute((__packed__)); // LegoRemote::Feedback
        
        static_assert(sizeof(Feedback) <= 32);

    }; // LegoRemote

}; // namespace remote