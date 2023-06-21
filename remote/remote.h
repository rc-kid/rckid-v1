#pragma once

#include "platform/platform.h"

namespace remote {

    /** Channel types supported by the remote protocol. 
     */
    enum class ChannelKind {
        Motor = 1, 
        CustomIO = 2, 
        ToneEffect = 3, 
        RGBStrip = 4, 
        RGBColor = 5,
    }; // remote::ChannelKind

    class MotorChannel {
    public:
        enum class Mode {
            Coast, 
            Brake, 
            CW, 
            CCW
        }; // MotorChannel::Mode

        struct Control {
            Mode mode;
            uint8_t speed;
        };

        struct Feedback {
            bool overcurrent;
            uint8_t v;
            uint8_t i;
        };

        struct Config {
            uint8_t overcurrent;
        };

        Control control;
        Feedback feedback;
        Config config;
    }; // remote::MotorChannel

    class CustomIOChannel {
    public:
        enum class Mode {
            DigitalOut = 0, 
            DigitalIn = 1, 
            AnalogOut = 2, 
            AnalogIn = 3, 
            PWM = 4, 
            Servo = 5, 
            Tone = 6, 
        }; 

        struct Control {
            uint16_t value;
        }; 

        struct Feedback {
            uint8_t value;
        };

        struct Config {
            Mode mode;
            uint16_t servoStart;
            uint16_t servoEnd;
        };

        Control control;
        Feedback feedback;
        Config config;

    }; // remote::CustomIOChannel

    class ToneEffectChannel {
    public:
        struct Control {

        }; 
        struct Feedback {

        };
        struct Config {

        };
    }; // remote::ToneEffectChannel

    class RGBStripChannel {
    public:
        struct Control {

        }; 
        struct Feedback {

        };
        struct Config {

        };
    }; // remote::RGBStripChannel

    class RGBColorChannel {
    public:
        struct Control {

        }; 
        struct Feedback {

        };
        struct Config {

        };
    }; // remote::RGBColorChannel

} // namespace remote